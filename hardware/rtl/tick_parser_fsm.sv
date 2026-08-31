`timescale 1ns / 1ps

module tick_parser_fsm (
    input  logic        clk,        // Reloj maestro del sistema (100 MHz)
    input  logic        rst_n,      // Reset activo a nivel bajo
    input  logic [ 7:0] rx_data,    // Byte procedente del módulo uart_rx
    input  logic        rx_valid,   // Pulso de 1 ciclo indicando byte válido
    output logic [63:0] price_out,  // Registro de 64 bits con el precio reconstruido
    output logic [ 7:0] type_out,   // Tipo de orden ('B' = 0x42, 'A' = 0x41)
    output logic        data_valid  // Pulso de 1 ciclo indicando paquete íntegro y listo
);
  // Byte magico de sincronismo segun el protocolo definido (protocol.h)
  localparam logic [7:0] MAGIC_BYTE = 8'h54;  // 'T' en ASCII

  // Estados de las maquinas deserializadora
  typedef enum logic [1:0] {
    ST_WAIT_MAGIC = 2'b00,  // Espera pasiva del delimitador de cabecera 'T'
    ST_READ_TYPE  = 2'b01,  // Captura de la dirección de orden ('B' / 'A')
    ST_READ_PRICE = 2'b10,  // Reconstrucción iterativa de los 8 bytes del precio
    ST_VALIDATE   = 2'b11   // Emisión síncrona de las señales estables
  } parser_state_t;

  parser_state_t state;

  logic [2:0] byte_counter;  // Contador de 0 a 7 para indexar los 8 bytes del precio
  logic [7:0] reg_type;  // Registro interno para almacenar el tipo
  logic [63:0] reg_price;  // Registro interno acomulador del precio (64 bits)

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state        <= ST_WAIT_MAGIC;
      byte_counter <= 3'd0;
      reg_type     <= 8'd0;
      reg_price    <= 64'd0;
      price_out    <= 64'd0;
      type_out     <= 8'd0;
      data_valid   <= 1'b0;
    end else begin
      // Por defecto, data_valid es 0 en cada ciclo
      data_valid <= 1'b0;

      case (state)
        ST_WAIT_MAGIC: begin
          byte_counter <= 3'd0;
          // Solo avanzamos si llega un byte válido y coincide con 'T'
          if (rx_valid && (rx_data == MAGIC_BYTE)) begin
            state <= ST_READ_TYPE;
          end
        end

        ST_READ_TYPE: begin
          if (rx_valid) begin
            reg_type <= rx_data; // Almacenar 'B' (Bid) o 'A' (Ask)
            state    <= ST_READ_PRICE;
          end
        end

        ST_READ_PRICE: begin
          if (rx_valid) begin
            // Dynamic Part-Select: ensambla de LSB a MSB (Little Endian de x86_64)
            reg_price[(byte_counter*8)+:8] <= rx_data;

            if (byte_counter == 3'd7) begin
              // Se han capturado los 8 bytes del double IEEE 754
              state <= ST_VALIDATE;
            end else begin
              byte_counter <= byte_counter + 1'b1;
            end
          end
        end

        ST_VALIDATE: begin
          // Presentar datos consolidados en los puertos de salida
          price_out <= reg_price;
          type_out <= reg_type;
          data_valid <= 1'b1;  // Pulso estroboscópico para el compute_core

          // Retornar inmediatamente a esperar el siguiente paquete
          state <= ST_WAIT_MAGIC;
        end

        default: state <= ST_WAIT_MAGIC;
      endcase
    end
  end
endmodule
