/*
 * hs_spi.h
 *
 *  Created on: 2019Äê12ÔÂ23ÈÕ
 *      Author: jace
 */

#ifndef DRIVER_HS_SPI_H_
#define DRIVER_HS_SPI_H_

void hs_spi_init();
int spi_write_data(uint8_t *wdat,size_t size);
int spi_read_data(uint8_t *rdat,size_t size);

int spi_req_lock();
int spi_req_unlock();

#endif /* DRIVER_HS_SPI_H_ */
