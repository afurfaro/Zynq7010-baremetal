void kernel_main(void)
{
    volatile unsigned int x = 0x12345678;

    while (1)
    {
        x++;
    }
}