# Análise de Compatibilidade: rusEFI vs Teensy 3.5 Implementation

**Data:** 2026-02-12
**Versão:** 2.3.0

---

## 📊 Resumo Executivo

| Componente | Portado | Simplificado | Omitido | Status |
|------------|---------|--------------|---------|--------|
| **Trigger Decoder** | 85% | 10% | 5% | ⚠️ Funcional, mas simplificado |
| **RPM Calculator** | 90% | 5% | 5% | ✅ Quase completo |
| **Event Scheduler** | 75% | 15% | 10% | ⚠️ Básico, faltam recursos |

---

## 🔍 Análise Detalhada

### 1️⃣ **Trigger Decoder** (TriggerDecoderBase)

#### ✅ O que foi portado fielmente:

1. **Algoritmo de detecção de dente ausente**
   - ✅ Comparação de ratio (current_period / previous_period)
   - ✅ Sync ratio configurável (1.5 a 3.0)
   - ✅ Rastreamento de posição de dente

2. **Estado de sincronização**
   - ✅ `sync_locked` flag
   - ✅ Contadores de sync/loss
   - ✅ Timestamp da última sincronização

3. **Callbacks**
   - ✅ `on_sync_callback` - chamado quando sincroniza
   - ✅ `on_tooth_callback` - chamado em cada dente

#### ⚠️ O que foi simplificado:

1. **Padrões de trigger**
   - Nossa implementação: **Apenas missing tooth** (36-1, 60-2, etc.)
   - rusEFI original: **60+ padrões diferentes**
     - Ford EDIS (36-1)
     - Honda K-series (12+1)
     - Mazda Miata NB (4+1)
     - Nissan SR20 (360 teeth)
     - GM 24x
     - Suzuki Vitara (2+2+2+1)
     - E muitos outros...

2. **TriggerShape (struct complexa)**
   - rusEFI: Suporta múltiplas bordas (rising/falling/both por dente)
   - Nossa implementação: Apenas uma borda por evento

3. **Trigger State Machine**
   - rusEFI: State machine completa com estados intermediários
   - Nossa implementação: Sincronizado ou não sincronizado (binário)

#### ❌ O que foi omitido:

1. **Suporte a sensor de came (cam sync)**
   - rusEFI: Lógica completa de sincronização crank+cam
   - Nossa implementação: ❌ Não implementado
   - **Impacto:** Não consegue distinguir ciclos de 720° (necessário para injeção sequencial)

2. **VVT (Variable Valve Timing) position**
   - rusEFI: Rastreamento de posição VVT
   - Nossa implementação: ❌ Não implementado

3. **Trigger error detection avançada**
   - rusEFI: Detecção de:
     - Jitter (variação de timing)
     - Noise (picos falsos)
     - Missing teeth consecutivos
     - Acceleration compensation
   - Nossa implementação: ⚠️ Apenas rejeição básica de noise (MIN_TOOTH_PERIOD)

4. **Trigger logging e debugging**
   - rusEFI: Sistema de logging detalhado
   - Nossa implementação: ❌ Não implementado

---

### 2️⃣ **RPM Calculator** (RpmCalculator)

#### ✅ O que foi portado fielmente:

1. **Cálculo de RPM instantâneo**
   - ✅ `instant_rpm = 60,000,000 / (tooth_period * teeth_per_rev)`
   - ✅ Idêntico ao rusEFI

2. **Filtro de média móvel exponencial**
   - ✅ `filtered_rpm = instant * alpha + old * (1 - alpha)`
   - ✅ Coeficiente configurável (default: 0.05)
   - ✅ Idêntico ao rusEFI

3. **Detecção de timeout**
   - ✅ Engine stopped detection
   - ✅ Timeout configurável (default: 1 segundo)

4. **Revolution-based RPM**
   - ✅ `rpm_calculator_on_revolution()`
   - ✅ Cálculo por revolução completa

#### ⚠️ O que foi simplificado:

1. **Smoothing variations**
   - rusEFI: Diferentes modos de smoothing (cranking, running, deceleration)
   - Nossa implementação: Um único coeficiente de filtro

2. **RPM validity checks**
   - rusEFI: Validação extensa (min/max RPM, aceleração impossível, etc.)
   - Nossa implementação: ⚠️ Validação básica

#### ❌ O que foi omitido:

1. **Spinning-up vs Spinning-down detection**
   - rusEFI: Detecta aceleração vs desaceleração
   - Nossa implementação: ❌ Não implementado
   - **Impacto:** Filtro não se adapta dinamicamente

2. **Cranking RPM mode**
   - rusEFI: Modo especial para cranking (partida)
   - Nossa implementação: ❌ Não implementado
   - **Impacto:** Pode ser mais lento para sincronizar na partida

3. **No-load vs Load RPM tracking**
   - rusEFI: Rastreia RPM com/sem carga
   - Nossa implementação: ❌ Não implementado

4. **Revolution time averaging**
   - rusEFI: Média de múltiplas revoluções
   - Nossa implementação: ⚠️ Apenas uma revolução

---

### 3️⃣ **Event Scheduler** (Event Queue)

#### ✅ O que foi portado fielmente:

1. **Algoritmo de conversão ângulo-para-tempo**
   - ✅ `us_per_degree = 60,000,000 / (rpm * 360)`
   - ✅ Idêntico ao rusEFI

2. **Agendamento baseado em ângulo**
   - ✅ Cálculo de angle_delta com wrap-around (720°)
   - ✅ `scheduled_time = current_time + (angle_delta * us_per_degree)`

3. **Event queue básica**
   - ✅ Array de eventos (16 slots)
   - ✅ Processamento de eventos por tempo

4. **Estatísticas**
   - ✅ Events scheduled/fired/missed tracking

#### ⚠️ O que foi simplificado:

1. **Priority system**
   - rusEFI: Sistema de prioridades (high/medium/low)
   - Nossa implementação: ❌ Sem prioridades (FIFO simples)
   - **Impacto:** Ignição pode não ter prioridade sobre injeção

2. **Event queue structure**
   - rusEFI: Heap-based priority queue
   - Nossa implementação: Array linear simples
   - **Impacto:** O(n) para buscar, rusEFI é O(log n)

3. **Angle prediction**
   - rusEFI: Predição de ângulo futuro baseado em aceleração
   - Nossa implementação: ❌ Assume RPM constante
   - **Impacto:** Menos preciso durante aceleração/desaceleração

#### ❌ O que foi omitido:

1. **Hardware timer scheduling**
   - rusEFI: Usa hardware timers (PIT) para agendamento preciso
   - Nossa implementação: ❌ Polling-based (precisa chamar `scheduler_process_events()`)
   - **Impacto:** ⚠️ **CRÍTICO** - Timing menos preciso, depende da frequência do main loop

2. **Multi-stage events**
   - rusEFI: Eventos com múltiplos estágios (início, meio, fim)
   - Nossa implementação: ❌ Apenas eventos únicos
   - **Impacto:** Não pode fazer:
     - Injection (start + end)
     - Dwell time (coil charge + spark)
     - Sequential operations

3. **Event cancellation/rescheduling**
   - rusEFI: Pode cancelar/reagendar eventos individuais
   - Nossa implementação: ⚠️ Apenas por cilindro (`scheduler_remove_cylinder_events`)
   - **Impacto:** Menos flexível

4. **Angle-based vs Time-based scheduling**
   - rusEFI: Suporta ambos (ângulo para alta RPM, tempo para baixa RPM)
   - Nossa implementação: ⚠️ Apenas ângulo
   - **Impacto:** Pode ter problemas em RPM muito baixo (<100 RPM)

5. **Event overlap detection**
   - rusEFI: Detecta conflitos (ex: injetor ainda aberto)
   - Nossa implementação: ❌ Não implementado
   - **Impacto:** Pode agendar eventos sobrepostos

---

## 🎯 Funcionalidades Críticas Faltando

### **ALTA PRIORIDADE** ⚠️

1. **Hardware timer integration no Event Scheduler**
   ```c
   // rusEFI usa PIT (Periodic Interrupt Timer)
   // Nossa implementação precisa:
   // - Integrar com FTM hardware timers
   // - Callback automático no tempo exato
   // - Não depender de polling
   ```
   **Impacto:** Timing de injeção/ignição impreciso

2. **Cam sync para injeção sequencial**
   ```c
   // Necessário para distinguir:
   // - Ciclo 1 (TDC compressão)
   // - Ciclo 2 (TDC escape)
   ```
   **Impacto:** Sem isso, apenas injeção simultânea/wasted spark

3. **Multi-stage events (injection start/end)**
   ```c
   // Injeção precisa de:
   // - Evento 1: Abrir injetor (turn ON)
   // - Evento 2: Fechar injetor (turn OFF)
   // Timing preciso entre os dois
   ```
   **Impacto:** Não pode controlar duração de injeção

### **MÉDIA PRIORIDADE** ⚙️

4. **Trigger patterns avançados**
   - Suporte para outros padrões além de missing-tooth
   - Estado machine completo

5. **Acceleration compensation**
   - Ajuste de timing durante aceleração
   - Predição de ângulo futuro

6. **Cranking mode**
   - RPM filtering especial para partida
   - Sincronização mais rápida

### **BAIXA PRIORIDADE** 📋

7. **VVT tracking**
8. **Trigger logging/debugging**
9. **Advanced error detection**
10. **Priority scheduling**

---

## 💡 Recomendações

### **Opção A: Usar como está (Protótipo)** ✅

**Prós:**
- ✅ Funciona para missing-tooth wheels (36-1, 60-2)
- ✅ RPM calculation preciso e suave
- ✅ Base sólida para desenvolvimento

**Contras:**
- ⚠️ Timing de eventos não é preciso (polling-based)
- ⚠️ Sem injeção sequencial (sem cam sync)
- ⚠️ Sem controle de duração (sem multi-stage events)

**Recomendado para:**
- Testes de bancada
- Desenvolvimento de algoritmos
- Validação de conceitos

---

### **Opção B: Adicionar funcionalidades críticas** 🔧

**Implementar:**

1. **Hardware timer scheduling** (CRÍTICO)
   ```c
   // Integrar com FTM timers do Teensy 3.5
   void scheduler_setup_hardware_timer(void);
   void ftm_timer_isr(void); // Callback automático
   ```

2. **Multi-stage events** (CRÍTICO)
   ```c
   // Adicionar suporte para eventos de início/fim
   typedef struct {
       uint16_t start_angle;
       uint16_t end_angle;
       // ou
       uint16_t start_angle;
       uint32_t duration_us;
   } multi_stage_event_t;
   ```

3. **Cam sync básico** (IMPORTANTE)
   ```c
   // Adicionar ao trigger decoder
   bool cam_state;
   uint8_t cycle_phase; // 0 = first 360°, 1 = second 360°
   ```

**Resultado:**
- ✅ Sistema funcional de injeção/ignição
- ✅ Timing preciso
- ✅ Injeção sequencial possível

---

### **Opção C: Port completo do rusEFI** 🚀

**Portar:**
- Todos os 60+ trigger patterns
- Sistema completo de VVT
- Scheduling com prioridades
- State machine completo

**Tempo estimado:** 2-3 semanas
**Complexidade:** Alta

**Resultado:**
- ✅ ECU production-ready
- ✅ Compatível com qualquer motor
- ✅ Todos os recursos do rusEFI

---

## 📋 Checklist de Funcionalidades

### Trigger Decoder
- [x] Missing tooth detection (basic)
- [x] Sync lock/loss tracking
- [x] Tooth position tracking
- [ ] Cam sync support
- [ ] Multiple trigger patterns
- [ ] VVT position tracking
- [ ] Advanced error detection
- [ ] Acceleration compensation

### RPM Calculator
- [x] Instantaneous RPM
- [x] Exponential filtering
- [x] Timeout detection
- [x] Revolution-based RPM
- [ ] Cranking mode
- [ ] Acceleration detection
- [ ] Multi-revolution averaging
- [ ] Spinning-up/down detection

### Event Scheduler
- [x] Angle-to-time conversion
- [x] Basic event scheduling
- [x] Event queue (16 slots)
- [x] Statistics tracking
- [ ] **Hardware timer integration** ⚠️ CRÍTICO
- [ ] **Multi-stage events** ⚠️ CRÍTICO
- [ ] Priority system
- [ ] Angle prediction
- [ ] Event overlap detection
- [ ] Time-based scheduling (low RPM)

---

## 🎯 Decisão Necessária

**Você precisa decidir o caminho:**

1. **Protótipo (como está)**
   - Seguir para integração e testes
   - Aceitar limitações conhecidas

2. **Adicionar funcionalidades críticas**
   - Hardware timer scheduling
   - Multi-stage events
   - Cam sync básico
   - ~3-5 dias de trabalho

3. **Port completo**
   - Sistema production-ready
   - ~2-3 semanas de trabalho

**Qual opção você prefere?** 🤔

