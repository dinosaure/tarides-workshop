enum {
    PERIPHERAL_BASE = 0xFE000000,
    GPFSEL0         = PERIPHERAL_BASE + 0x200000,
    GPSET0          = PERIPHERAL_BASE + 0x20001C,
    GPCLR0          = PERIPHERAL_BASE + 0x200028,
    GPPUPPDN0       = PERIPHERAL_BASE + 0x2000E4
};

enum {
    GPIO_MAX_PIN       = 53,
    GPIO_FUNCTION_ALT5 = 2
};

enum {
    Pull_None = 0
};

void mmio_write(long reg, unsigned int val) { *(volatile unsigned int *)reg = val; }
unsigned int mmio_read(long reg) { return *(volatile unsigned int *)reg; }

unsigned int gpio_call(unsigned int pin_number, unsigned int value, unsigned int base, unsigned int field_size, unsigned int field_max) {
    unsigned int field_mask = (1 << field_size) - 1;
  
    if (pin_number > field_max) return 0;
    if (value > field_mask) return 0; 

    unsigned int num_fields = 32 / field_size;
    unsigned int reg = base + ((pin_number / num_fields) * 4);
    unsigned int shift = (pin_number % num_fields) * field_size;

    unsigned int curval = mmio_read(reg);
    curval &= ~(field_mask << shift);
    curval |= value << shift;
    mmio_write(reg, curval);

    return 1;
}

unsigned int gpio_set     (unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPSET0, 1, GPIO_MAX_PIN); }
unsigned int gpio_clear   (unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPCLR0, 1, GPIO_MAX_PIN); }
unsigned int gpio_pull    (unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPPUPPDN0, 2, GPIO_MAX_PIN); }
unsigned int gpio_function(unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPFSEL0, 3, GPIO_MAX_PIN); }

void gpio_use_as_alt5(unsigned int pin_number) {
    gpio_pull(pin_number, Pull_None);
    gpio_function(pin_number, GPIO_FUNCTION_ALT5);
}

enum {
    AUX_BASE        = PERIPHERAL_BASE + 0x215000,
    AUX_IRQ         = AUX_BASE,
    AUX_ENABLES     = AUX_BASE + 4,
    AUX_MU_IO_REG   = AUX_BASE + 64,
    AUX_MU_IER_REG  = AUX_BASE + 68,
    AUX_MU_IIR_REG  = AUX_BASE + 72,
    AUX_MU_LCR_REG  = AUX_BASE + 76,
    AUX_MU_MCR_REG  = AUX_BASE + 80,
    AUX_MU_LSR_REG  = AUX_BASE + 84,
    AUX_MU_MSR_REG  = AUX_BASE + 88,
    AUX_MU_SCRATCH  = AUX_BASE + 92,
    AUX_MU_CNTL_REG = AUX_BASE + 96,
    AUX_MU_STAT_REG = AUX_BASE + 100,
    AUX_MU_BAUD_REG = AUX_BASE + 104,
    AUX_UART_CLOCK  = 500000000,
    UART_MAX_QUEUE  = 16 * 1024
};

#define AUX_MU_BAUD(baud) ((AUX_UART_CLOCK/(baud*8))-1)

void uart_init() {
    mmio_write(AUX_ENABLES, 1); //enable UART1
    mmio_write(AUX_MU_IER_REG, 0);
    mmio_write(AUX_MU_CNTL_REG, 0);
    mmio_write(AUX_MU_LCR_REG, 3); //8 bits
    mmio_write(AUX_MU_MCR_REG, 0);
    mmio_write(AUX_MU_IER_REG, 0);
    mmio_write(AUX_MU_IIR_REG, 0xC6); //disable interrupts
    mmio_write(AUX_MU_BAUD_REG, AUX_MU_BAUD(115200));
    gpio_use_as_alt5(14);
    gpio_use_as_alt5(15);
    mmio_write(AUX_MU_CNTL_REG, 3); //enable RX/TX
}

unsigned int uart_is_write_byte_ready() { return mmio_read(AUX_MU_LSR_REG) & 0x20; }

void uart_write_byte_blocking_actual(unsigned char ch) {
    while (!uart_is_write_byte_ready()); 
    mmio_write(AUX_MU_IO_REG, (unsigned int)ch);
}

void uart_write_text(const char *buffer) {
    while (*buffer) {
       if (*buffer == '\n') uart_write_byte_blocking_actual('\r');
       uart_write_byte_blocking_actual(*buffer++);
    }
}

#include <stddef.h>

void uart_puts(const char *buffer, size_t len) {
  while (len--) {
       if (*buffer == '\n') uart_write_byte_blocking_actual('\r');
       uart_write_byte_blocking_actual(*buffer++);
  }
}

#include "printf_config.h"

typedef enum {
  ERROR= 0,
  WARN,
  INFO,
  DEBUG
} log_level_t;

static log_level_t log_level = DEBUG;

int log(log_level_t level, const char *fmt, ...)
{
  if (log_level < level)
    return 0;

  va_list args;
  size_t  size;
  char buffer[320];

  va_start(args, fmt);
  size = vsnprintf(buffer, sizeof buffer, fmt, args);
  va_end(args);

  if (size >= sizeof buffer) {
    const char trunc[] = "(truncated)\n";
    uart_puts(buffer, sizeof buffer - 1);
    uart_puts(trunc, sizeof trunc - 1);
    return sizeof buffer - 1;
  } else {
    uart_puts(buffer, size);
    return size;
  }
}

#include <stdint.h>

static inline uint64_t cpu_cntpct(void)
{
  uint64_t val;

  __asm__ __volatile__("mrs %0, cntpct_el0" : "=r" (val)::);
  return val;
}

static inline uint64_t cpu_cntfrq(void)
{
  uint64_t val;

  __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(val)::);
  return val;
}

#define NSEC_PER_SEC 1000000000ULL

static uint64_t time_base;
static uint64_t tsc_base;
static uint8_t  tsc_shift;
static uint32_t tsc_mult;

void mclock_init(void)
{
  uint64_t tsc_freq = cpu_cntfrq();
  tsc_shift = 32;
  uint64_t tmp;

  do {
    tmp = (NSEC_PER_SEC << tsc_shift) / tsc_freq;
    if ((tmp & 0xFFFFFFFF00000000L) == 0L)
      tsc_mult = (uint32_t)tmp;
    else
      tsc_shift--;
  } while (tsc_shift > 0 && tsc_mult == 0L);

  tsc_base = cpu_cntpct();
  time_base = (tsc_base * tsc_mult) >> tsc_shift;
}

uint64_t mclock(void)
{
  uint64_t tsc_now, tsc_delta;

  tsc_now = cpu_cntpct();
  tsc_delta = tsc_now - tsc_base;
  time_base += (tsc_delta * tsc_mult) >> tsc_shift;
  tsc_base = tsc_now;

  return time_base;
}

#define PAGE_SIZE	4096
#define PAGE_SHIFT	12
#define PAGE_MASK	~(0xfff)

#define MEMORY_SIZE	0x30000000 /* 768Mb */

static uint64_t heap_start;

void mem_lock_heap(uintptr_t *start, size_t *size)
{
    *start = heap_start;
    *size = MEMORY_SIZE - heap_start;
}

void mem_init(void)
{
    extern char _stext[], _etext[], _erodata[], _end[];
    uint64_t mem_size;

    mem_size = MEMORY_SIZE;
    heap_start = ((uint64_t)&_end + PAGE_SIZE - 1) & PAGE_MASK;

    log(INFO, "RPi4: Memory map: %llu MB addressable:\n",
            (unsigned long long)mem_size >> 20);
    log(INFO, "RPi4:   reserved @ (0x0 - 0x%llx)\n",
            (unsigned long long)_stext-1);
    log(INFO, "RPi4:       text @ (0x%llx - 0x%llx)\n",
            (unsigned long long)_stext, (unsigned long long)_etext-1);
    log(INFO, "RPi4:     rodata @ (0x%llx - 0x%llx)\n",
            (unsigned long long)_etext, (unsigned long long)_erodata-1);
    log(INFO, "RPi4:       data @ (0x%llx - 0x%llx)\n",
            (unsigned long long)_erodata, (unsigned long long)_end-1);
    log(INFO, "RPi4:       heap >= 0x%llx < stack < 0x%llx\n",
            (unsigned long long)heap_start, (unsigned long long)mem_size);
}

extern void _nolibc_init(uintptr_t heap_start, size_t heap_size);
extern int printf(const char *, ...);

void main() {
  uintptr_t heap_start;
  size_t    heap_size;

  uart_init();
  mclock_init();

  log(INFO, " _____ _ _ _           _ _           \n");
  log(INFO, "|   __|_| | |_ ___ ___| | |_ ___ ___ \n");
  log(INFO, "|  |  | | | . |  _| .'| |  _| .'|  _|\n");
  log(INFO, "|_____|_|_|___|_| |__,|_|_| |__,|_|  \n");

  mem_init();
  mem_lock_heap(&heap_start, &heap_size);

  _nolibc_init(heap_start, heap_size);
  printf("Hello World from C!\n");
  printf("[0] Current monotonic clock: %016ld.\n", mclock());
  printf("[1] Current monotonic clock: %016ld.\n", mclock());

  for(;;);
}
