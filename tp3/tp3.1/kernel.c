void kernel_main(void)
{
    gic_init();
    private_timer_init();

    while (1)
    {
    }
}