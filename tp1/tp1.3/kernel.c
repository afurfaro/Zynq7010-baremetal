int initialized = 42;
int counter;

void kernel_main(void)
{
    while (1) {
        initialized++;
        counter++;
    }
}