`timescale 1ns / 1ps

module uart_rx #(
    // Parametro configurable
    parameter int CLKS_PER_BIT = 868
) (
    input  logic       clk,      // Reloj del sistema
    input  logic       rst_n,    // Reset global asincrono a nivel bajo
    input  logic       rx,       // Pin fisico de entrada UART (asincrono)
    output logic [7:0] rx_data,  // Byte deserializado
    output logic       rx_valid  // Pulso de 1 ciclo indicando byte listo
);

  // Estados de la FSM como enum, al ser 4 estados se utilizaran 2 bits [1:0]
  typedef enum logic [1:0] {
    IDLE = 2'b00,  // Linia en reposo
    START_BIT = 2'b01,  // Deteccion y validacion del bit Start
    DATA_BITS = 2'b10,  // Captura de los 8 bits de datos (LSB a MSB)
    STOP_BIT = 2'b11  // Espera del bit de parada
  } uart_rx_state;

  uart_rx_state state;

  //Registros internos
  logic [15:0] clk_count;  // Contador temporal para medir el tiempo del bit
  logic [2:0] bit_index;  // Indice del bit actual que se esta recibiendo
  logic [7:0] rx_byte;  // Registro de desplazamiento para reconstruir el byte
  logic rx_sync_0;  // Primer flip-flop de sincronizacion
  logic rx_sync;  // Segundo flip-flop de sincronizacion (señal limpia)

  // Sincronizador de 2 etapas para mitigar Metaestabilidad
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      rx_sync_0 <= 1'b1;  // UART en reposo siempre es nivel alto
      rx_sync   <= 1'b1;
    end else begin
      rx_sync_0 <= rx;
      rx_sync   <= rx_sync_0;
    end
  end

  // Máquina de Estados (FSM) de Recepción UART
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state <= IDLE;
      clk_count <= 16'b0;
      bit_index <= 3'd0;
      rx_byte <= 8'd0;
      rx_data <= 8'd0;
      rx_valid <= 1'b0;
    end else begin
      // Por defecto, rx_valid es 0 cada ciclo. Solo durará 1 ciclo cuando se active.
      rx_valid <= 1'b0;

      case (state)
        IDLE: begin
          clk_count <= 16'd0;
          bit_index <= 3'd0;
          // Detectar flanco de bajada (inicio de transmision en serie)
          if (!rx_sync) begin
            state <= START_BIT;
          end
        end

        START_BIT: begin
          // Muestreamos en el centro exacto del bit de Start para verificar validez
          if (clk_count == (CLKS_PER_BIT / 2) - 1) begin
            if (!rx_sync) begin
              // Confirmado: es un bit de Start real (0). Reiniciamos contador.
              clk_count <= 16'd0;
              state <= DATA_BITS;
            end else begin
              // Falsa alarma (glitch/ruido)
              state <= IDLE;
            end
          end else begin
            clk_count <= clk_count + 1'b1;
          end
        end

        DATA_BITS: begin
          // Contar hasta el centro del siguiente bit de datos
          if (clk_count < CLKS_PER_BIT - 1) begin
            clk_count <= clk_count + 1'b1;
          end else begin
            clk_count <= 16'd0;
            rx_byte[bit_index] <= rx_sync;  // Muestrear y guardar el bit recibido

            if (bit_index < 3'd7) begin
              bit_index <= bit_index + 1'b1;
            end else begin
              bit_index <= 3'd0;
              state <= STOP_BIT;
            end
          end
        end

        STOP_BIT: begin
          // Esperar el tiempo correspondiente al bit Stop
          if (clk_count < CLKS_PER_BIT - 1) begin
            clk_count <= clk_count + 1'b1;
          end else begin
            clk_count <= 16'b0;
            rx_data   <= rx_byte;  // Publicar el byte recibido completo
            rx_valid  <= 1'b1;  // Disparar strobe de validez (1 ciclo)
            state     <= IDLE;
          end
        end
        default: state <= IDLE;
      endcase
    end
  end





endmodule
