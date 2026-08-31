## =============================================================================
## RESTRICCIONES FÍSICAS Y DE TIEMPO: PROYECTO HFT
## Target: Digilent Nexys A7-100T (Xilinx Artix-7 xc7a100tcsg324-1)
## =============================================================================

## Reloj Maestro del Sistema (100 MHz / 10 ns)
set_property -dict { PACKAGE_PIN E3    IOSTANDARD LVCMOS33 } [get_ports { clk_100mhz }];
create_clock -add -name sys_clk_pin -period 10.000 -waveform {0.000 5.000} [get_ports { clk_100mhz }];

## Reset Global (Botón Rojo CPU_RESETN - Activo a nivel bajo)
set_property -dict { PACKAGE_PIN C12   IOSTANDARD LVCMOS33 } [get_ports { btn_rst_n }];

## Enlace Serie USB-UART (Chip FTDI FT2232HQ)
## - RX: Entrada de datos serie hacia la FPGA (desde FTDI TX)
## - TX: Salida de datos serie hacia el host (hacia FTDI RX)
set_property -dict { PACKAGE_PIN C4    IOSTANDARD LVCMOS33 } [get_ports { uart_rx_in }];
set_property -dict { PACKAGE_PIN D4    IOSTANDARD LVCMOS33 } [get_ports { uart_tx_out }];

## Telemetría Visual: LEDs LD0 a LD15
set_property -dict { PACKAGE_PIN H17   IOSTANDARD LVCMOS33 } [get_ports { led[0] }];  # LD0: Tick Ingest Pulse
set_property -dict { PACKAGE_PIN K15   IOSTANDARD LVCMOS33 } [get_ports { led[1] }];  # LD1: Order Book Ready
set_property -dict { PACKAGE_PIN J13   IOSTANDARD LVCMOS33 } [get_ports { led[2] }];  # LD2: Order Fire Trigger
set_property -dict { PACKAGE_PIN N14   IOSTANDARD LVCMOS33 } [get_ports { led[3] }];  # LD3: Packet Builder Busy
set_property -dict { PACKAGE_PIN R18   IOSTANDARD LVCMOS33 } [get_ports { led[4] }];  # LD4: UART TX Busy
set_property -dict { PACKAGE_PIN V17   IOSTANDARD LVCMOS33 } [get_ports { led[5] }];  # LD5
set_property -dict { PACKAGE_PIN U17   IOSTANDARD LVCMOS33 } [get_ports { led[6] }];  # LD6
set_property -dict { PACKAGE_PIN U16   IOSTANDARD LVCMOS33 } [get_ports { led[7] }];  # LD7
set_property -dict { PACKAGE_PIN V16   IOSTANDARD LVCMOS33 } [get_ports { led[8] }];  # LD8: Action[0]
set_property -dict { PACKAGE_PIN T15   IOSTANDARD LVCMOS33 } [get_ports { led[9] }];  # LD9: Action[1]
set_property -dict { PACKAGE_PIN U14   IOSTANDARD LVCMOS33 } [get_ports { led[10] }]; # LD10: Action[2]
set_property -dict { PACKAGE_PIN T16   IOSTANDARD LVCMOS33 } [get_ports { led[11] }]; # LD11: Action[3]
set_property -dict { PACKAGE_PIN V15   IOSTANDARD LVCMOS33 } [get_ports { led[12] }]; # LD12: Action[4]
set_property -dict { PACKAGE_PIN V14   IOSTANDARD LVCMOS33 } [get_ports { led[13] }]; # LD13: Action[5]
set_property -dict { PACKAGE_PIN V12   IOSTANDARD LVCMOS33 } [get_ports { led[14] }]; # LD14: Action[6]
set_property -dict { PACKAGE_PIN V11   IOSTANDARD LVCMOS33 } [get_ports { led[15] }]; # LD15: Action[7]

## Configuración de Voltaje y Bitstream
set_property CFGBVS VCCO        [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]