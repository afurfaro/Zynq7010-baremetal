void kernel_main(void)
{
    asm volatile("svc #0");
    while(1);
}