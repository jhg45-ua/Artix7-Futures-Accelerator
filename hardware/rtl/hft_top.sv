`timescale 1ns / 1ps

module hft_top #(
    parameter int CLKS_PER_BIT = 868  // 100 MHz / 115200 baudios
) (
    input  logic        clk_100mhz,   // Pin E3 (Oscilador 100 MHz Nexys A7)
    input  logic        btn_rst_n,    // Pin C12 (CPU_RESETN activo a nivel bajo)
    input  logic        uart_rx_in,   // Pin C4 (RX desde el chip FT2232HQ)
    output logic        uart_tx_out,  // Pin D4 (TX hacia el chip FT2232HQ)
    output logic [15:0] led           // Pines H17..V11 (LEDs LD0 a LD15)
);

  // Cables Internos de Interconexión (Puntos Críticos del Pipeline)
  // Canal Ingesta: uart_rx -> tick_parser_fsm
  logic [ 7:0] rx_byte;
  logic        rx_valid;

  // Canal Decodificado: tick_parser_fsm -> compute_core
  logic [63:0] parsed_price;
  logic [ 7:0] parsed_type;
  logic        parsed_valid;

  // Canal Decisión: compute_core -> order_packet_builder
  logic [63:0] best_bid;
  logic [63:0] best_ask;
  logic        book_ready;
  logic        order_fire;
  logic [ 7:0] order_action;

  // Canal Retorno: order_packet_builder -> uart_tx
  logic        tx_start;
  logic [ 7:0] tx_data;
  logic        tx_busy;
  logic        tx_done;
  logic        packet_busy;

  // 2. Instanciación del Pipeline Completo

  // Bloque 1: Receptor Serie (Capa Física)
  uart_rx #(
      .CLKS_PER_BIT(CLKS_PER_BIT)
  ) u_uart_rx (
      .clk     (clk_100mhz),
      .rst_n   (btn_rst_n),
      .rx      (uart_rx_in),
      .rx_data (rx_byte),
      .rx_valid(rx_valid)
  );

  // Bloque 2: Parser Deserializador (Capa Protocolo de Entrada)
  tick_parser_fsm u_parser (
      .clk       (clk_100mhz),
      .rst_n     (btn_rst_n),
      .rx_data   (rx_byte),
      .rx_valid  (rx_valid),
      .price_out (parsed_price),
      .type_out  (parsed_type),
      .data_valid(parsed_valid)
  );

  // Bloque 3: Motor Algorítmico (Capa de Cómputo)
  compute_core u_compute (
      .clk             (clk_100mhz),
      .rst_n           (btn_rst_n),
      .in_tick_valid   (parsed_valid),
      .in_order_type   (parsed_type),
      .in_price        (parsed_price),
      .out_best_bid    (best_bid),
      .out_best_ask    (best_ask),
      .out_book_ready  (book_ready),
      .out_order_fire  (order_fire),
      .out_order_action(order_action)
  );

  // Bloque 4: Empaquetador de Órdenes (Capa Protocolo de Salida)
  order_packet_builder u_pkt_builder (
      .clk            (clk_100mhz),
      .rst_n          (btn_rst_n),
      .in_order_fire  (order_fire),
      .in_order_action(order_action),
      .tx_busy        (tx_busy),
      .tx_done        (tx_done),
      .tx_start       (tx_start),
      .tx_data        (tx_data),
      .out_packet_busy(packet_busy)
  );

  // Bloque 5: Transmisor Serie (Capa Física de Retorno)
  uart_tx #(
      .CLKS_PER_BIT(CLKS_PER_BIT)
  ) u_uart_tx (
      .clk     (clk_100mhz),
      .rst_n   (btn_rst_n),
      .tx_start(tx_start),
      .tx_data (tx_data),
      .tx      (uart_tx_out),
      .tx_busy (tx_busy),
      .tx_done (tx_done)
  );

  // 3. Telemetría y Diagnóstico Visual (LEDs Nexys A7)

  // Los pulsos de 10 ns son invisibles al ojo humano. 
  // Implementamos un ensanchador de pulso (pulse stretcher) para visualizar eventos.
  logic [23:0] pulse_stretch_tick;
  logic [23:0] pulse_stretch_fire;
  logic [ 7:0] latched_action;

  always_ff @(posedge clk_100mhz or negedge btn_rst_n) begin
    if (!btn_rst_n) begin
      pulse_stretch_tick <= 24'd0;
      pulse_stretch_fire <= 24'd0;
      latched_action     <= 8'd0;
    end else begin
      // Contador de actividad de entrada (~100 ms de encendido)
      if (parsed_valid) begin
        pulse_stretch_tick <= 24'd10_000_000;
      end else if (pulse_stretch_tick > 0) begin
        pulse_stretch_tick <= pulse_stretch_tick - 1'b1;
      end

      // Contador de disparos de orden (~200 ms de encendido)
      if (order_fire) begin
        pulse_stretch_fire <= 24'd20_000_000;
        latched_action     <= order_action;
      end else if (pulse_stretch_fire > 0) begin
        pulse_stretch_fire <= pulse_stretch_fire - 1'b1;
      end
    end
  end

  // Mapeo de indicadores
  assign led[0]    = (pulse_stretch_tick > 0);  // LD0: Actividad de Ticks recibidos
  assign led[1]    = book_ready;  // LD1: Top of Book listo (Bid + Ask)
  assign led[2]    = (pulse_stretch_fire > 0);  // LD2: ¡Disparo de Orden Algorítmica!
  assign led[3]    = packet_busy;  // LD3: Paquete de salida en transmisión
  assign led[4]    = tx_busy;  // LD4: UART TX ocupada físicamente
  assign led[7:5]  = 3'b000;  // Reservados
  assign led[15:8] = latched_action;  // LD15..LD8: Código ASCII de última acción ('B'/'A')

endmodule
