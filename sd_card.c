#include "sd_card.h"
#include "gpio.h"
#include "spi.h"
#include "debug.h"
#include "string.h"
#include <stdint.h>

#define SD_SPI  hspi2
#define SD_SPI_TIMEOUT 100
#define SD_SPI_PORT SD_CS_GPIO_Port
#define SD_SPI_PIN  SD_CS_Pin

#define DELAY(ms) HAL_Delay(ms)

/**
 * @brief CRC7 calculation
 * @param pack ptr to pack
 * @param size size of pack
 * @return uint8_t 
 */
static inline uint8_t CRC7(uint8_t * pack, uint16_t size){
    uint8_t crc = 0x00;
    while(size--){
        crc ^= *(pack++);
        for (uint8_t iter = 0; iter < 8; iter++){
            crc = (crc & 0x80u) ? ((crc << 1) ^ (CRC7_POLYNOMIAL << 1)) : (crc << 1);
        }
    }
    return (crc | 0x01);
}

/**
 * @brief SD Card SPI Select
 */
static inline void SDSPISelect(void){
    HAL_GPIO_WritePin(SD_SPI_PORT, SD_SPI_PIN, 0);
}

/**
 * @brief SD Card SPI Deselect
 */
static inline void SDSPIDeselect(void){
    HAL_GPIO_WritePin(SD_SPI_PORT, SD_SPI_PIN, 1);
}

/**
 * @brief Wait for SD Card to Process the data
 * @param timeout timeout for waiting and excluding inf loop
 * @return sd_card_status_t 
 */
static inline sd_card_status_t SDCardWaitForReady(uint32_t timeout){
    uint8_t rx_byte = 0x00;
    uint8_t dummy = 0xFF;
    uint32_t ticks = HAL_GetTick();
    while(HAL_GetTick() - ticks < timeout){
        HAL_SPI_TransmitReceive(&SD_SPI, &dummy, &rx_byte, 1, SD_SPI_TIMEOUT);
        if (rx_byte == 0xFF) return SD_OK;
    }
    return SD_TIMEOUT;
}

/**
 * @brief SD Card transmit data
 * @param p_data data
 * @param size size
 */
static inline sd_card_status_t SDCardTransmitData(uint8_t * p_data, uint16_t size){
    if (HAL_SPI_Transmit(&SD_SPI, p_data, size, 100) != HAL_OK)
        return SD_TX_ERROR;
    else return SD_OK;
}

/**
 * @brief SD Card Receive data
 * @param p_data data
 * @param size size
 */
static inline sd_card_status_t SDCardReceiveData(uint8_t * p_data, uint16_t size){
    uint8_t dummy_tx = 0xFF;
    for (uint16_t data_ptr = 0; data_ptr < size; data_ptr++){
        if (HAL_SPI_TransmitReceive(&SD_SPI, &dummy_tx, p_data + data_ptr, 1, SD_SPI_TIMEOUT) != HAL_OK)
            return SD_RX_ERROR;
    }
    return SD_OK;
}

/**
 * @brief Get R1 response
 * @return R1 structure 
 */
static inline R1_fields_t SDCardGetR1(uint32_t timeout){
    R1_fields_t r1 = 0xFF;
    uint8_t dummy = 0xFF;
    uint32_t ticks = HAL_GetTick();
    while(HAL_GetTick() - ticks < timeout){
        HAL_SPI_TransmitReceive(&SD_SPI, &dummy, &r1, 1, 1000);
        if (r1 != 0xFF)
            break;
    }
    if (r1 & (R1_BUSY)) return R1_BUSY;
    if (r1 & (R1_PARAMETER_ERROR)) return R1_PARAMETER_ERROR;
    if (r1 & (R1_ADDRESS_ERROR)) return R1_ADDRESS_ERROR;
    if (r1 & (R1_ERASE_SEQUENCE_ERROR)) return R1_ERASE_SEQUENCE_ERROR;
    if (r1 & (R1_COMMAND_CRC_ERROR)) return R1_COMMAND_CRC_ERROR;
    if (r1 & (R1_ILLEGAL_COMMAND)) return R1_ILLEGAL_COMMAND;
    if (r1 & (R1_ERASE_RESET)) return R1_ERASE_RESET;
    if (r1 & (R1_IN_IDLE_STATE)) return R1_IN_IDLE_STATE;
    else return 0x00;
}

/**
 * @brief Get R7 response
 * @return R7 argument
 */
static inline uint32_t SDCardGetR7(uint32_t timeout){
    uint32_t r7 = 0xFFFFFFFF;
    uint8_t dummy[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    if (SDCardGetR1(timeout) > R1_IN_IDLE_STATE)
        return r7;
    HAL_SPI_TransmitReceive(&SD_SPI, dummy, &r7, sizeof(r7), 1000);
    return r7;
}


sd_card_status_t SDCard_Init(void){
    sd_card_frame_t cmd;
    uint8_t dummy_tx = 0xFF;
    // Sending 80 CLK pulses
    SDSPIDeselect();
    for (uint8_t iter = 0; iter < 10; iter++)
        HAL_SPI_Transmit(&SD_SPI, &dummy_tx, sizeof(dummy_tx), SD_SPI_TIMEOUT);
    SDSPISelect();

    // CMD0(0x00000000)
    cmd.command = SD_CMD0_GO_IDLE_STATE;
    cmd.arg = 0;
    cmd.crc7 = CRC7((uint8_t *)&cmd, sizeof(cmd) - 1);
    
    if (SDCardWaitForReady(0xFFFF) != SD_OK)
        return SD_TIMEOUT;

    HAL_SPI_Transmit(&hspi2, &cmd, sizeof(cmd), 100);
    if (SDCardGetR1(1000) != R1_IN_IDLE_STATE)
        return SD_ERROR;

    // CMD8(0x000001AA)
    cmd.command = SD_CMD8_SEND_IF_COND;
    cmd.arg = inv32(0x000001AA);
    cmd.crc7 = CRC7((uint8_t *)&cmd, sizeof(cmd) - 1);

    if (SDCardWaitForReady(0xFFFF) != SD_OK)
        return SD_TIMEOUT;

    HAL_SPI_Transmit(&hspi2, &cmd, sizeof(cmd), 100);
    if (SDCardGetR7(1000) == inv32(0x000001AA))
        print_in("SD Card Type: SD Ver. 2\r\n");
    else
        print_wr("SD Card Type: MMC, SD Ver. 1 or Unknown\r\n");

    // CMD55(0x00000000) with CMD41(0x40000000)
    uint32_t ticks = HAL_GetTick();
    while(1){
        cmd.command = SD_CMD55_APP_CMD;
        cmd.arg = inv32(0x00000000);
        cmd.crc7 = CRC7((uint8_t *)&cmd, sizeof(cmd) - 1);
        if (SDCardWaitForReady(0xFFFF) != SD_OK)
            return SD_TIMEOUT;

        if (HAL_SPI_Transmit(&hspi2, &cmd, sizeof(cmd), 100) != HAL_OK)
            return SD_TX_ERROR;

        if (SDCardGetR1(1000) != R1_IN_IDLE_STATE)
            return SD_ERROR;

        cmd.command = SD_CMD41;
        cmd.arg = inv32(0x40000000);
        cmd.crc7 = CRC7((uint8_t *)&cmd, sizeof(cmd) - 1);
        if (SDCardWaitForReady(0xFFFF) != SD_OK)
            return SD_TIMEOUT;

        if (HAL_SPI_Transmit(&hspi2, &cmd, sizeof(cmd), 100) != HAL_OK)
            return SD_TX_ERROR;

        if (SDCardGetR1(1000) == 0x00)
            break;

        if (HAL_GetTick() - ticks > 5000) return SD_TIMEOUT;
    }
    
    // CMD58(0x00000000)
    cmd.command = SD_CMD58_READ_OCR;
    cmd.arg = inv32(0x00000000);
    cmd.crc7 = CRC7((uint8_t *)&cmd, sizeof(cmd) - 1);

    if (SDCardWaitForReady(0xFFFF) != SD_OK)
        return SD_TIMEOUT;

    if (HAL_SPI_Transmit(&hspi2, &cmd, sizeof(cmd), 100) != HAL_OK)
        return SD_TX_ERROR;
    
    // If OCR(30) is not set, then CMD16(0x00000200)
    if (inv32(SDCardGetR7(1000)) & (1 << 30) == 0){
        print_in("SD Card Standard Capacity\r\n");
        // cmd.command = SD_CMD16_SET_BLOCKLEN;
        // cmd.arg = inv32(0x00000200);
        // cmd.crc7 = 0xFF;

        // if (SDCardWaitForReady(1000) != SD_OK)
        //     return SD_TIMEOUT;

        // HAL_SPI_Transmit(&hspi2, &cmd, sizeof(cmd), 100);

        // if (SDCardGetR1(1000) != 0x00)
        //     return SD_ERROR;
    }
    else{
        print_in("SD Card Extended Capacity\r\n");
        // cmd.command = SD_CMD16_SET_BLOCKLEN;
        // cmd.arg = inv32(0x00000200);
        // cmd.crc7 = 0xFF;

        // if (SDCardWaitForReady(1000) != SD_OK)
        //     return SD_TIMEOUT;

        // HAL_SPI_Transmit(&hspi2, &cmd, sizeof(cmd), 100);

        // if (SDCardGetR1(1000) != 0x00)
        //     return SD_ERROR;
    }
    SDSPIDeselect();

    // SD_SPI.Init.BaudRatePrescaler = 

    return SD_OK;
}

sd_card_status_t SD_WriteData(uint8_t * p_data, uint32_t addr, uint32_t blocks_am){
  while(blocks_am--){
    if (SDCardWriteBlock(p_data, addr) != SD_OK) return SD_ERROR;
    p_data += 512;
    addr++;
  }
  return SD_OK;
}

sd_card_status_t SD_ReadData(uint8_t * p_data, uint32_t addr, uint32_t blocks_am){
  while(blocks_am--){
    if (SDCardReadBlock(p_data, addr) != SD_OK) return SD_ERROR;
    p_data += 512;
    addr++;
  }
  return SD_OK;
}

static inline sd_card_status_t SDCardWaitToken(uint8_t token, uint32_t timeout){
    uint8_t recv_token = 0x00;
    uint8_t dummy_tx = 0xFF;
    uint32_t ticks = HAL_GetTick();
    while(HAL_GetTick() - ticks < timeout){
        HAL_SPI_TransmitReceive(&hspi2, &dummy_tx, &recv_token, 1, 100);
        if (recv_token == token)
            return SD_OK;
        if (recv_token != 0xFF)
            return SD_ERROR;
    }
    return SD_TIMEOUT;
}

sd_card_status_t SDCardWriteBlock(uint8_t * p_data, uint32_t addr){
    uint8_t Token = SD_TOKEN_24;
    uint8_t dummy_crc[2] = { 0xFF, 0xFF };
    sd_card_frame_t cmd = {
        .command = SD_CMD24_WRITE_BLOCK,
        .arg = addr,
        .crc7 = 0xFF //CRC7(&cmd, sizeof(cmd) - 1)
    };
    SDSPISelect();
    if (SDCardWaitForReady(1000) != SD_OK)
        return SD_TIMEOUT;

    if (SDCardTransmitData((uint8_t *)&cmd, sizeof(cmd)) != SD_OK)
        return SD_TX_ERROR;
    
    if (SDCardGetR1(1000) != 0x00)
        return SD_ERROR;
    
    if (SDCardTransmitData(&Token, 1) != SD_OK)
        return SD_TX_ERROR;
    
    if (SDCardTransmitData(p_data, SD_BLOCK_SIZE) != SD_OK)
        return SD_TX_ERROR;
    
    if (SDCardTransmitData(dummy_crc, 2) != SD_OK)
        return SD_TX_ERROR;

    uint8_t resp_data;
    if (SDCardReceiveData(&resp_data, sizeof(resp_data)) != SD_OK)
        return SD_RX_ERROR;

    if ((resp_data & SD_WRITE_RESPONSE_MASK) != SD_WRITE_RESPONSE_ACCEPTED)
        return SD_ERROR;

    if (SDCardWaitForReady(1000) != SD_OK)
        return SD_TIMEOUT;

    SDSPIDeselect();
    return SD_OK;
}

sd_card_status_t SDCardReadBlock(uint8_t * p_data, uint32_t addr){
    uint8_t Token = SD_TOKEN_17;
    uint8_t recv_crc[2];
    sd_card_frame_t cmd = {
        .command = SD_CMD17_READ_SINGLE_BLOCK,
        .arg = addr,
        .crc7 = 0xFF
    };

    SDSPISelect();
    if (SDCardWaitForReady(1000) != SD_OK)
        return SD_TIMEOUT;

    if (SDCardTransmitData((uint8_t *)&cmd, sizeof(cmd)) != SD_OK)
        return SD_TX_ERROR;

    if (SDCardGetR1(1000) != 0x00)
        return SD_ERROR;

    switch(SDCardWaitToken(Token, 1000)){
        case(SD_ERROR):
            return SD_ERROR;
        case(SD_TIMEOUT):
            return SD_TIMEOUT;
    }

    if (SDCardReceiveData(p_data, SD_BLOCK_SIZE) != SD_OK)
        return SD_RX_ERROR;
    
    if (SDCardReceiveData(recv_crc, 2) != SD_OK)
        return SD_RX_ERROR;

    SDSPIDeselect();
    return SD_OK;
}

sd_card_status_t SDCardStartMultipleWriting(uint32_t start_addr){
    sd_card_frame_t cmd = {
        .command = SD_CMD25_WRITE_MULTIPLE_BLOCK,
        .arg = start_addr,
        .crc7 = 0xFF
    };
    uint8_t dummy = 0xFF;
    SDSPISelect();
    if (SDCardWaitForReady(1000) != SD_OK)
        return SD_TIMEOUT;
    
    if (SDCardTransmitData((uint8_t *)&cmd, sizeof(cmd)) != SD_OK)
        return SD_TX_ERROR;

    if (SDCardGetR1(1000) != 0x00)
        return SD_ERROR;

    if (SDCardWaitForReady(1000) != SD_OK)
        return SD_TIMEOUT;
    SDSPIDeselect();

    return SD_OK;
}

sd_card_status_t SDCardPushBlock(uint8_t * p_data){
    sd_card_tokens_t Token = SD_TOKEN_25;
    uint8_t dummy_crc[2] = { 0xFF, 0xFF };
    SDSPISelect();
    if (SDCardWaitForReady(1000) != SD_OK)
        return SD_TIMEOUT;

    if (SDCardTransmitData(&Token, 1) != SD_OK)
        return SD_TX_ERROR;

    if (SDCardTransmitData(p_data, SD_BLOCK_SIZE) != SD_OK)
        return SD_TX_ERROR;

    if (SDCardTransmitData(dummy_crc, 2) != SD_OK)
        return SD_TX_ERROR;
    
    uint8_t resp_data;
    if (SDCardReceiveData(&resp_data, sizeof(resp_data)) != SD_OK)
        return SD_RX_ERROR;

    if ((resp_data & SD_WRITE_RESPONSE_MASK) != SD_WRITE_RESPONSE_ACCEPTED)
        return SD_ERROR;
    
    if (SDCardWaitForReady(1000) != SD_OK)
        return SD_TIMEOUT;
    
    SDSPIDeselect();
    return SD_OK;
}

sd_card_status_t SDCardStopMultipleWriting(void){
    sd_card_tokens_t Token = SD_TOKEN_25_STOP;
    uint8_t dummy = 0xFF;
    SDSPISelect();
    if (SDCardTransmitData(&Token, 1) != SD_OK){
        SDSPIDeselect();
        return SD_TX_ERROR;
    }

    if (SDCardTransmitData(&dummy, 1) != SD_OK){
    // if (SDCardReceiveData(&Token, 1) != SD_OK){
        SDSPIDeselect();
        return SD_RX_ERROR;
    }

    if (SDCardWaitForReady(1000) != SD_OK){
        SDSPIDeselect();
        return SD_TIMEOUT;
    }

    SDSPIDeselect();
    return SD_OK;
}