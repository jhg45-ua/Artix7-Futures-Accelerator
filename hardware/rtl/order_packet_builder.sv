`timescale 1ns / 1ps

module order_packet_builder (
    input logic clk,   // Reloj maestro de 100 MHz
    input logic rst_n, // Reset activo a nivel bajo

    // Interfaz de disparo desde compute_core
    input logic       in_order_fire,   // Pulso de 1 ciclo
    input logic [7:0] in_order_action, // 'B' o 'A'

    // Interfaz de control hacia uart_tx
    input  logic       tx_busy,   // Estado del transmisor UART
    input  logic       tx_done,   // Pulso de byte completado
    output logic       tx_start,  // Pulso para transmitir byte
    output logic [7:0] tx_data,   // Byte a transmitir

    // Telemetría de estado
    output logic out_packet_busy  // 1 mientras el paquete de 10 bytes se transmite
);

  localparam logic [7:0] MAGIC_ORDER = 8'h4F;  // 'O' en ASCII

  typedef enum logic [1:0] {
    ST_IDLE      = 2'b00,
    ST_SEND_BYTE = 2'b01,
    ST_WAIT_BYTE = 2'b10
  } builder_state_t;

  builder_state_t state;

  // Contadores internos de telemetría y secuenciación
  logic [31:0] free_running_timer;  // Contador libre a 100 MHz (1 tick = 10 ns)
  logic [31:0] order_counter;  // ID autoincremental de orden
  logic [3:0] byte_index;  // Índice de 0 a 9 para iterar el paquete
  logic [7:0] packet_buffer[0:9];  // Buffer de los 10 bytes empaquetados

  assign out_packet_busy = (state != ST_IDLE);

  // Contador de Ciclos (Hardware Timestamping)
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      free_running_timer <= 32'd0;
    end else begin
      free_running_timer <= free_running_timer + 32'd1;
    end
  end

  // FSM de Empaquetado y Despacho Byte a Byte
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state         <= ST_IDLE;
      order_counter <= 32'd0;
      byte_index    <= 4'd0;
      tx_start      <= 1'b0;
      tx_data       <= 8'd0;
      for (int i = 0; i < 10; i++) begin
        packet_buffer[i] <= 8'd0;
      end
    end else begin
      tx_start <= 1'b0;  // Default strobe

      case (state)
        ST_IDLE: begin
          byte_index <= 4'd0;

          // Captura de evento de disparo (si la UART no está ocupada)
          if (in_order_fire && !tx_busy) begin
            order_counter <= order_counter + 32'd1;

            // Byte 0: Delimitador de inicio
            packet_buffer[0] <= MAGIC_ORDER;

            // Byte 1: Acción de trading
            packet_buffer[1] <= in_order_action;

            // Bytes 2 a 5: Order ID (Little Endian)
            packet_buffer[2] <= order_counter[7:0];
            packet_buffer[3] <= order_counter[15:8];
            packet_buffer[4] <= order_counter[23:16];
            packet_buffer[5] <= order_counter[31:24];

            // Bytes 6 a 9: Hardware Timestamp a 10 ns (Little Endian)
            packet_buffer[6] <= free_running_timer[7:0];
            packet_buffer[7] <= free_running_timer[15:8];
            packet_buffer[8] <= free_running_timer[23:16];
            packet_buffer[9] <= free_running_timer[31:24];

            state <= ST_SEND_BYTE;
          end
        end

        ST_SEND_BYTE: begin
          tx_data  <= packet_buffer[byte_index];
          tx_start <= 1'b1; // Disparar UART TX para este byte
          state    <= ST_WAIT_BYTE;
        end

        ST_WAIT_BYTE: begin
          // Esperar a que uart_tx complete el byte (pulso tx_done)
          if (tx_done) begin
            if (byte_index == 4'd9) begin
              state <= ST_IDLE;  // Paquete completo de 10 bytes enviado
            end else begin
              byte_index <= byte_index + 4'd1;
              state      <= ST_SEND_BYTE;
            end
          end
        end

        default: state <= ST_IDLE;
      endcase
    end
  end

endmodule
