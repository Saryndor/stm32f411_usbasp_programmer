#pragma once


void uart2_dma_init(void);
void uart2_dma_start(void);
void uart2_drain_to_cdc0(void);
void cdc0_drain_to_uart2(void);
void uart2_tx_dma_done(void);                            