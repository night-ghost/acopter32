/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
/*
 *       DataFlash.cpp - DataFlash log library generic code
 */

#include <AP_HAL/AP_HAL.h>
#include "DataFlash.h"

extern AP_HAL::HAL& hal;

// the last page holds the log format in first 4 bytes. Please change
// this if (and only if!) the low level format changes
#define DF_LOGGING_FORMAT    0x28122013


#if 0 &&  CONFIG_HAL_BOARD == HAL_BOARD_REVOMINI

#include "DataFlash.h"

extern AP_HAL::HAL& hal;

/*
 * TODO DRIVER NEED TO BE FIXED !!!
 */

// the last page holds the log format in first 4 bytes. Please change
// this if (and only if!) the low level format changes
#define DF_LOGGING_FORMAT    0x28122013

#define dfEE_PAGESIZE   256
volatile uint8_t   dfEEDataNum;


// *** DATAFLASH PUBLIC FUNCTIONS ***
void DataFlash_Block::StartWrite(uint16_t PageAdr)
{
    df_BufferIdx  = 0;
    df_BufferNum  = 0;
    df_PageAdr    = PageAdr;

    if (df_PageAdr >= DF_LAST_PAGE){
	df_PageAdr = 1;
	Flash_Jedec_EraseSector(0); // Erase Sector
    }

    uint16_t data = 0;
    BlockRead((df_PageAdr << 8), &data, sizeof(data));
    if (data != 0xFFFF)
       Flash_Jedec_EraseSector(df_PageAdr << 8); // Erase Sector
}

void DataFlash_Block::FinishWrite(void)
{
    WaitReady();

    BufferToPage (df_PageAdr << 8); // Save 256 bytes from buffer to Page

    df_PageAdr++;

    if (df_PageAdr > df_NumPages){
	Flash_Jedec_EraseSector(0); // Erase Sector 0
    	df_PageAdr = 1;
    }
    // check if new sector is erased
    if (df_PageAdr % 256 == 0){
	uint16_t data = 0;
	BlockRead((df_PageAdr << 8), &data, sizeof(data));
    	if (data != 0xFFFF){
    	   Flash_Jedec_EraseSector(df_PageAdr << 8); // Erase Sector
        }
    }

    df_BufferIdx = 0;
}

void DataFlash_Block::WriteBlock(const void *pBuffer, uint16_t size)
{
    if (ReadStatus() != 0)
	return;

    if (!CardInserted() || !log_write_started) {
        return;
    }

    while (size > 0) {
	uint16_t n = df_PageSize - df_BufferIdx;
	if (n > size) {
	    n = size;
	}

	if (df_BufferIdx == 0) {
	    // if we are at the start of a page we need to insert a
	    // page header
	    if (n > df_PageSize - sizeof(struct PageHeader)) {
		n = df_PageSize - sizeof(struct PageHeader);
	    }
	    struct PageHeader ph = { df_FileNumber, df_FilePage };
	    BlockWrite(df_BufferIdx, &ph, sizeof(ph), pBuffer, n); // write PageHeader+data
	    df_BufferIdx += n + sizeof(ph);
	 } else {
	    BlockWrite(df_BufferIdx, NULL, 0, pBuffer, n);
	    df_BufferIdx += n;
	 }

	size -= n;
	pBuffer = (const void *)(n + (uintptr_t)pBuffer);

	if (df_BufferIdx == df_PageSize) {
	    FinishWrite();
	    df_FilePage++;
	}
    }
}


// Get the last page written to
uint16_t DataFlash_Block::GetWritePage()
{
    return df_PageAdr;
}

// Get the last page read
uint16_t DataFlash_Block::GetPage()
{
    return df_Read_PageAdr;
}

void DataFlash_Block::StartRead(uint16_t PageAdr)
{
    df_Read_PageAdr   = PageAdr;

    // We are starting a new page - read FileNumber and FilePage
    struct PageHeader ph;
    BlockRead(df_Read_PageAdr << 8, &ph, sizeof(ph));
    df_FileNumber = ph.FileNumber;
    df_FilePage   = ph.FilePage;
    df_Read_BufferIdx = sizeof(ph);
}

bool DataFlash_Block::ReadBlock(void *pBuffer, uint16_t size)
{
    while (size > 0) {
        uint16_t n = df_PageSize - df_Read_BufferIdx;
        if (n > size) {
            n = size;
        }

        BlockRead(df_Read_BufferIdx + (df_Read_PageAdr << 8), pBuffer, n);
        size -= n;
        pBuffer = (void *)(n + (uintptr_t)pBuffer);

        df_Read_BufferIdx += n;

        if (df_Read_BufferIdx == df_PageSize) { // df_PageSize=256 bytes
            df_Read_PageAdr++;
            if (df_Read_PageAdr > df_NumPages) {
                df_Read_PageAdr = 1;
            }
            // We are starting a new page - read FileNumber and FilePage
            struct PageHeader ph;
            BlockRead(df_Read_PageAdr << 8, &ph, sizeof(ph));
            df_FileNumber = ph.FileNumber;
            df_FilePage   = ph.FilePage;

            df_Read_BufferIdx = sizeof(ph);
        }
    }
    return true;
}

void DataFlash_Block::SetFileNumber(uint16_t FileNumber)
{
    df_FileNumber = FileNumber;
    df_FilePage = 1;
}

uint16_t DataFlash_Block::GetFileNumber()
{
    return df_FileNumber;
}

uint16_t DataFlash_Block::GetFilePage()
{
    return df_FilePage;
}

#define LED_GRN (*((unsigned long int *) 0x42408294)) // PB5
#define LED_YLW (*((unsigned long int *) 0x42408298)) // PB6 // not instaled
#define LED_RED (*((unsigned long int *) 0x42408290)) // PB4
#include <delay.h>

void DataFlash_Block::Erase_Sectors(uint8_t start, uint8_t end)
{
    static uint16_t _erase_led = 0;
    LED_GRN = 0;
    LED_RED = 1;
    // Erase XX sectors * 256 bytes
    for (uint16_t sector = start; sector < end; sector++) {
	Flash_Jedec_EraseSector(sector << 16);
	WaitReady();
	 if (_erase_led == 1){
	     LED_RED=0;
	     LED_GRN = 1;
	     _erase_led=0;
	     } else{
	     LED_RED=1;
	     LED_GRN = 0;
             _erase_led=1;
	     }
    }
    LED_RED = 1;
    LED_GRN = 1;
}

void DataFlash_Block::EraseAll()
{
    Erase_Sectors(0,32); // Erase 32 sectors * 256 bytes

    // write the logging format in the first page
    hal.scheduler->delay(100);
    df_BufferIdx  = 0;
    df_BufferNum  = 0;
    df_PageAdr    = DF_LAST_PAGE;
    uint32_t version = DF_LOGGING_FORMAT;
    log_write_started = true;
    df_FileNumber = 1;
    df_FilePage = 1;
    WriteBlock(&version, sizeof(version));
    log_write_started = false;
    FinishWrite();
    hal.scheduler->delay(100);
}

/*
 *  we need to erase if the logging format has changed
 */
bool DataFlash_Block::NeedErase(void)
{
    uint32_t version = 0;

    StartRead(DF_LAST_PAGE);  // Last Page
    ReadBlock(&version, sizeof(version));
    StartRead(1); //1
    return version != DF_LOGGING_FORMAT;
}



int16_t DataFlash_Block::get_log_data_raw(uint16_t log_num, uint16_t page, uint32_t offset, uint16_t len, uint8_t *data)
{
    uint16_t data_page_size = df_PageSize - sizeof(struct PageHeader);

    if (offset >= data_page_size) {
        page += offset / data_page_size;
        offset = offset % data_page_size;
        if (page > df_NumPages) {
            // pages are one based, not zero
            page = 1 + page - df_NumPages;
        }
    }
    if (log_write_started || df_Read_PageAdr != page) {
        StartRead(page);
    }

    df_Read_BufferIdx = offset + sizeof(struct PageHeader);
    ReadBlock(data, len);

    return (int16_t)len;
}

/**
  get data from a log, accounting for adding FMT headers
 */

int16_t DataFlash_Block::get_log_data(uint16_t log_num, uint16_t page, uint32_t offset, uint16_t len, uint8_t *data)
{
    if (offset == 0) {
        uint8_t header[3];
        get_log_data_raw(log_num, page, 0, 3, header);
        adding_fmt_headers = (header[0] != HEAD_BYTE1 || header[1] != HEAD_BYTE2 || header[2] != LOG_FORMAT_MSG);
    }
    uint16_t ret = 0;

    if (adding_fmt_headers) {
        // the log doesn't start with a FMT message, we need to add
        // them
        const uint16_t fmt_header_size = _num_types * sizeof(struct log_Format);
        while (offset < fmt_header_size && len > 0) {
            struct log_Format pkt;
            uint8_t t = offset / sizeof(pkt);
            uint8_t ofs = offset % sizeof(pkt);
            Log_Fill_Format(&_structures[t], pkt);
            uint8_t n = sizeof(pkt) - ofs;
            if (n > len) {
                n = len;
            }
            memcpy(data, ofs + (uint8_t *)&pkt, n);
            data += n;
            offset += n;
            len -= n;
            ret += n;
        }
        offset -= fmt_header_size;
    }

    if (len > 0) {
        ret += get_log_data_raw(log_num, page, offset, len, data);
    }

    return ret;
}


/* END REVOMINI DATA FLASH */


#else

// *** DATAFLASH PUBLIC FUNCTIONS ***
void DataFlash_Block::StartWrite(uint16_t PageAdr)
{
    df_BufferIdx  = 0;
    df_BufferNum  = 0;
    df_PageAdr    = PageAdr;
    WaitReady();
}

void DataFlash_Block::FinishWrite(void)
{
    // Write Buffer to flash, NO WAIT
    BufferToPage(df_BufferNum, df_PageAdr, 0);      
    df_PageAdr++;
    // If we reach the end of the memory, start from the begining    
    if (df_PageAdr > df_NumPages)
        df_PageAdr = 1;

    // switch buffer
    df_BufferNum ^= 1;
    df_BufferIdx = 0;
}

void DataFlash_Block::WriteBlock(const void *pBuffer, uint16_t size)
{
    if (!CardInserted() || !log_write_started || !_writes_enabled) {
        return;
    }
    while (size > 0) {
        uint16_t n = df_PageSize - df_BufferIdx;
        if (n > size) {
            n = size;
        }

        if (df_BufferIdx == 0) {
            // if we are at the start of a page we need to insert a
            // page header
            if (n > df_PageSize - sizeof(struct PageHeader)) {
                n = df_PageSize - sizeof(struct PageHeader);
            }
            struct PageHeader ph = { df_FileNumber, df_FilePage };
            BlockWrite(df_BufferNum, df_BufferIdx, &ph, sizeof(ph), pBuffer, n);
            df_BufferIdx += n + sizeof(ph);
        } else {
            BlockWrite(df_BufferNum, df_BufferIdx, NULL, 0, pBuffer, n);
            df_BufferIdx += n;
        }

        size -= n;
        pBuffer = (const void *)(n + (uintptr_t)pBuffer);

        if (df_BufferIdx == df_PageSize) {
            FinishWrite();
            df_FilePage++;
        }
    }
}


// Get the last page written to
uint16_t DataFlash_Block::GetWritePage()
{
    return df_PageAdr;
}

// Get the last page read
uint16_t DataFlash_Block::GetPage()
{
    return df_Read_PageAdr;
}

void DataFlash_Block::StartRead(uint16_t PageAdr)
{
    df_Read_BufferNum = 0;
    df_Read_PageAdr   = PageAdr;

    // disable writing while reading
    log_write_started = false;

    WaitReady();

    // copy flash page to buffer
    PageToBuffer(df_Read_BufferNum, df_Read_PageAdr);

    // We are starting a new page - read FileNumber and FilePage
    struct PageHeader ph;
    BlockRead(df_Read_BufferNum, 0, &ph, sizeof(ph));
    df_FileNumber = ph.FileNumber;
    df_FilePage   = ph.FilePage;
    df_Read_BufferIdx = sizeof(ph);
}

bool DataFlash_Block::ReadBlock(void *pBuffer, uint16_t size)
{
    while (size > 0) {
        uint16_t n = df_PageSize - df_Read_BufferIdx;
        if (n > size) {
            n = size;
        }

        WaitReady();

        if (!BlockRead(df_Read_BufferNum, df_Read_BufferIdx, pBuffer, n)) {
            return false;
        }
        size -= n;
        pBuffer = (void *)(n + (uintptr_t)pBuffer);
        
        df_Read_BufferIdx += n;

        if (df_Read_BufferIdx == df_PageSize) {
            df_Read_PageAdr++;
            if (df_Read_PageAdr > df_NumPages) {
                df_Read_PageAdr = 1;
            }
            PageToBuffer(df_Read_BufferNum, df_Read_PageAdr);

            // We are starting a new page - read FileNumber and FilePage
            struct PageHeader ph;
            if (!BlockRead(df_Read_BufferNum, 0, &ph, sizeof(ph))) {
                return false;
            }
            df_FileNumber = ph.FileNumber;
            df_FilePage   = ph.FilePage;

            df_Read_BufferIdx = sizeof(ph);
        }
    }
    return true;
}

void DataFlash_Block::SetFileNumber(uint16_t FileNumber)
{
    df_FileNumber = FileNumber;
    df_FilePage = 1;
}

uint16_t DataFlash_Block::GetFileNumber()
{
    return df_FileNumber;
}

uint16_t DataFlash_Block::GetFilePage()
{
    return df_FilePage;
}

void DataFlash_Block::EraseAll()
{
    log_write_started = false;
    for (uint16_t j = 1; j <= (df_NumPages+1)/8; j++) {
        BlockErase(j);
        if (j%6 == 0) {
            hal.scheduler->delay(6);
        }
    }
    // write the logging format in the last page
    hal.scheduler->delay(100);
    StartWrite(df_NumPages+1);
    uint32_t version = DF_LOGGING_FORMAT;
    log_write_started = true;
    _writes_enabled = true;
    WriteBlock(&version, sizeof(version));
    log_write_started = false;
    FinishWrite();
    hal.scheduler->delay(100);
}

/*
 *  we need to erase if the logging format has changed
 */
bool DataFlash_Block::NeedErase(void)
{
    uint32_t version = 0;
    StartRead(df_NumPages+1);
    if (!ReadBlock(&version, sizeof(version))) {
        return true;
    }
    StartRead(1);
    return version != DF_LOGGING_FORMAT;
}

/**
  get raw data from a log
 */
int16_t DataFlash_Block::get_log_data_raw(uint16_t log_num, uint16_t page, uint32_t offset, uint16_t len, uint8_t *data)
{
    uint16_t data_page_size = df_PageSize - sizeof(struct PageHeader);

    if (offset >= data_page_size) {
        page += offset / data_page_size;
        offset = offset % data_page_size;
        if (page > df_NumPages) {
            // pages are one based, not zero
            page = 1 + page - df_NumPages;
        }
    }
    if (log_write_started || df_Read_PageAdr != page) {
        StartRead(page);
    }

    df_Read_BufferIdx = offset + sizeof(struct PageHeader);
    if (!ReadBlock(data, len)) {
        return -1;
    }

    return (int16_t)len;
}

/**
  get data from a log, accounting for adding FMT headers
 */
int16_t DataFlash_Block::get_log_data(uint16_t log_num, uint16_t page, uint32_t offset, uint16_t len, uint8_t *data)
{
    if (offset == 0) {
        uint8_t header[3];
        get_log_data_raw(log_num, page, 0, 3, header);
        adding_fmt_headers = (header[0] != HEAD_BYTE1 || header[1] != HEAD_BYTE2 || header[2] != LOG_FORMAT_MSG);
    }
    uint16_t ret = 0;

    if (adding_fmt_headers) {
        // the log doesn't start with a FMT message, we need to add
        // them
        const uint16_t fmt_header_size = _num_types * sizeof(struct log_Format);
        while (offset < fmt_header_size && len > 0) {
            struct log_Format pkt;
            uint8_t t = offset / sizeof(pkt);
            uint8_t ofs = offset % sizeof(pkt);
            Log_Fill_Format(&_structures[t], pkt);
            uint8_t n = sizeof(pkt) - ofs;
            if (n > len) {
                n = len;
            }
            memcpy(data, ofs + (uint8_t *)&pkt, n);
            data += n;
            offset += n;
            len -= n;
            ret += n;
        }
        offset -= fmt_header_size;
    }

    if (len > 0) {
        ret += get_log_data_raw(log_num, page, offset, len, data);
    }

    return ret;
}

#endif

