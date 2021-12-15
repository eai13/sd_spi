#ifndef SD_CARD_STM_LIBRARY_H
#define SD_CARD_STM_LIBRARY_H

#include <stdint.h>
#include <spi.h>

#define inv16(data)         ((data >> 8) | (data << 8))
#define inv32(data)         (((data >> 24) & 0xFF) | ((data >> 8) & 0xFF00) | ((data << 8) & 0xFF0000) | ((data << 24) & 0xFF000000))

#define CRC7_POLYNOMIAL (uint8_t)0b10001001

#define SD_CMD_MASK (uint8_t)0b01000000

#define SD_BLOCK_SIZE 512

// #define SD_CRC7_ON

typedef enum sd_write_response{
    SD_WRITE_RESPONSE_MASK =        (uint8_t)(0b00001111),
    SD_WRITE_RESPONSE_ACCEPTED =    (uint8_t)(0b00000101),
    SD_WRITE_RESPONSE_CRC_ERROR =   (uint8_t)(0b00001011),
    SD_WRITE_RESPONSE_WRITE_ERROR = (uint8_t)(0b00001101)
}sd_write_response_t;

typedef enum sd_card_tokens{
    SD_TOKEN_17 =           (uint8_t)(0b11111110),
    SD_TOKEN_18 =           (uint8_t)(0b11111110),
    SD_TOKEN_24 =           (uint8_t)(0b11111110),
    SD_TOKEN_25 =           (uint8_t)(0b11111100),
    SD_TOKEN_25_STOP =      (uint8_t)(0b11111101),
    SD_TOKEN_ERROR =        (uint8_t)(0b00000001),
    SD_TOKEN_CCERROR =      (uint8_t)(0b00000010),
    SD_TOKEN_ECCFAIL =      (uint8_t)(0b00000100),
    SD_TOKEN_RNGERROR =     (uint8_t)(0b00001000),
    SD_TOKEN_CARDLOCKED =   (uint8_t)(0b00010000)
}sd_card_tokens_t;

/**
 * @brief SPI Command Set
 */
typedef enum sd_card_command{
    SD_CMD0_GO_IDLE_STATE =             (uint8_t)(SD_CMD_MASK | 0x00),
    SD_CMD1_SEND_OP_COND =              (uint8_t)(SD_CMD_MASK | 0x01),
    SD_CMD2 =                           (uint8_t)(SD_CMD_MASK | 0x02),    
    SD_CMD3 =                           (uint8_t)(SD_CMD_MASK | 0x03),
    SD_CMD4 =                           (uint8_t)(SD_CMD_MASK | 0x04),
    SD_CMD5 =                           (uint8_t)(SD_CMD_MASK | 0x05),  
    SD_CMD6_SWITCH_FUNC =               (uint8_t)(SD_CMD_MASK | 0x06),
    SD_CMD7 =                           (uint8_t)(SD_CMD_MASK | 0x07),
    SD_CMD8_SEND_IF_COND =              (uint8_t)(SD_CMD_MASK | 0x08),
    SD_CMD9_SEND_CSD =                  (uint8_t)(SD_CMD_MASK | 0x09),
    SD_CMD10_SEND_CID =                 (uint8_t)(SD_CMD_MASK | 0x0A),
    SD_CMD11 =                          (uint8_t)(SD_CMD_MASK | 0x0B),
    SD_CMD12_STOP_TRANSMISSION =        (uint8_t)(SD_CMD_MASK | 0x0C),
    SD_CMD13_SEND_STATUS =              (uint8_t)(SD_CMD_MASK | 0x0D),
    SD_CMD14 =                          (uint8_t)(SD_CMD_MASK | 0x0E),
    SD_CMD15 =                          (uint8_t)(SD_CMD_MASK | 0x0F),
    SD_CMD16_SET_BLOCKLEN =             (uint8_t)(SD_CMD_MASK | 0x10),
    SD_CMD17_READ_SINGLE_BLOCK =        (uint8_t)(SD_CMD_MASK | 0x11),
    SD_CMD18_READ_MULTIPLE_BLOCK =      (uint8_t)(SD_CMD_MASK | 0x12),
    SD_CMD19 =                          (uint8_t)(SD_CMD_MASK | 0x13),
    SD_CMD20 =                          (uint8_t)(SD_CMD_MASK | 0x14),
    SD_CMD21 =                          (uint8_t)(SD_CMD_MASK | 0x15),
    SD_CMD22 =                          (uint8_t)(SD_CMD_MASK | 0x16),   
    SD_CMD23_SET_BLOCK_COUNT =          (uint8_t)(SD_CMD_MASK | 0x17),
    SD_CMD24_WRITE_BLOCK =              (uint8_t)(SD_CMD_MASK | 0x18),
    SD_CMD25_WRITE_MULTIPLE_BLOCK =     (uint8_t)(SD_CMD_MASK | 0x19),
    SD_CMD26 =                          (uint8_t)(SD_CMD_MASK | 0x1A),
    SD_CMD27_PROGRAM_CSD =              (uint8_t)(SD_CMD_MASK | 0x1B),
    SD_CMD28_SET_WRITE_PROT =           (uint8_t)(SD_CMD_MASK | 0x1C),
    SD_CMD29_CLR_WRITE_PROT =           (uint8_t)(SD_CMD_MASK | 0x1D),
    SD_CMD30_SEND_WRITE_PROT =          (uint8_t)(SD_CMD_MASK | 0x1E),
    SD_CMD31 =                          (uint8_t)(SD_CMD_MASK | 0x1F),
    SD_CMD32_ERASE_WR_BLK_START_ADDR =  (uint8_t)(SD_CMD_MASK | 0x20),
    SD_CMD33_ERASE_WR_BLK_END_ADDR =    (uint8_t)(SD_CMD_MASK | 0x21),
    SD_CMD34 =                          (uint8_t)(SD_CMD_MASK | 0x22),
    SD_CMD35 =                          (uint8_t)(SD_CMD_MASK | 0x23),
    SD_CMD36 =                          (uint8_t)(SD_CMD_MASK | 0x24),
    SD_CMD37 =                          (uint8_t)(SD_CMD_MASK | 0x25),
    SD_CMD38_ERASE =                    (uint8_t)(SD_CMD_MASK | 0x26),
    SD_CMD39 =                          (uint8_t)(SD_CMD_MASK | 0x27),    
    SD_CMD40 =                          (uint8_t)(SD_CMD_MASK | 0x28),
    SD_CMD41 =                          (uint8_t)(SD_CMD_MASK | 0x29),
    SD_CMD42_LOCK_UNLOCK =              (uint8_t)(SD_CMD_MASK | 0x2A),
    SD_CMD43 =                          (uint8_t)(SD_CMD_MASK | 0x2B),
    SD_CMD44 =                          (uint8_t)(SD_CMD_MASK | 0x2C),
    SD_CMD45 =                          (uint8_t)(SD_CMD_MASK | 0x2D),
    SD_CMD46 =                          (uint8_t)(SD_CMD_MASK | 0x2E),
    SD_CMD47 =                          (uint8_t)(SD_CMD_MASK | 0x2F),
    SD_CMD48 =                          (uint8_t)(SD_CMD_MASK | 0x30),
    SD_CMD49 =                          (uint8_t)(SD_CMD_MASK | 0x31),
    SD_CMD50 =                          (uint8_t)(SD_CMD_MASK | 0x32),
    SD_CMD51 =                          (uint8_t)(SD_CMD_MASK | 0x33),
    SD_CMD52 =                          (uint8_t)(SD_CMD_MASK | 0x34),
    SD_CMD53 =                          (uint8_t)(SD_CMD_MASK | 0x35),
    SD_CMD54 =                          (uint8_t)(SD_CMD_MASK | 0x36),
    SD_CMD55_APP_CMD =                  (uint8_t)(SD_CMD_MASK | 0x37),
    SD_CMD56_GEN_CMD =                  (uint8_t)(SD_CMD_MASK | 0x38),
    SD_CMD57 =                          (uint8_t)(SD_CMD_MASK | 0x39),
    SD_CMD58_READ_OCR =                 (uint8_t)(SD_CMD_MASK | 0x3A),
    SD_CMD59_CRC_ON_OFF =               (uint8_t)(SD_CMD_MASK | 0x3B),
    SD_CMD60 =                          (uint8_t)(SD_CMD_MASK | 0x3C),
    SD_CMD61 =                          (uint8_t)(SD_CMD_MASK | 0x3D),
    SD_CMD62 =                          (uint8_t)(SD_CMD_MASK | 0x3E),
    SD_CMD63 =                          (uint8_t)(SD_CMD_MASK | 0x3F)    
}sd_card_command_t;

typedef enum sd_card_status{
    SD_OK =         0x00,
    SD_TIMEOUT =    0x01,
    SD_TX_ERROR =   0x02,
    SD_RX_ERROR =   0x03,
    SD_ERROR =      0x04
}sd_card_status_t;

typedef enum R1_fields{
    R1_IN_IDLE_STATE =          (1 << 0),
    R1_ERASE_RESET =            (1 << 1),
    R1_ILLEGAL_COMMAND =        (1 << 2),
    R1_COMMAND_CRC_ERROR =      (1 << 3),
    R1_ERASE_SEQUENCE_ERROR =   (1 << 4),
    R1_ADDRESS_ERROR =          (1 << 5),
    R1_PARAMETER_ERROR =        (1 << 6),
    R1_BUSY =                   (1 << 7)
}R1_fields_t;

/**
 * @brief R3 SD Card Response
 */
typedef struct R3_response{
    uint8_t R1;
    uint32_t arg;
}__attribute__((packed, aligned(1))) R3_response_t;

// typedef struct R7_response{
//     uint8_t R1;
//     uint32_t arg;
// }__attribute__((packed, aligned(1))) R7_response_t;

typedef struct sd_card_frame{
    uint8_t command;
    int32_t arg;
    uint8_t crc7;
}__attribute__((packed, aligned(1))) sd_card_frame_t;

sd_card_status_t SDCard_Init(void);

sd_card_status_t SD_WriteData(uint8_t * p_data, uint32_t addr, uint32_t blocks_am);
sd_card_status_t SD_ReadData(uint8_t * p_data, uint32_t addr, uint32_t blocks_am);

sd_card_status_t SDCardWriteBlock(uint8_t * p_data, uint32_t addr);
sd_card_status_t SDCardReadBlock(uint8_t * p_data, uint32_t addr);

sd_card_status_t SDCardStartMultipleWriting(uint32_t start_addr);
sd_card_status_t SDCardPushBlock(uint8_t * p_data);
sd_card_status_t SDCardStopMultipleWriting(void);
#endif