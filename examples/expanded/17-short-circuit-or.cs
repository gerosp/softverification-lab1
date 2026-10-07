// 17. Короткое замыкание ||
// Цель: Правая часть || вычисляется только при ложной левой.
// В графе: При divisor = 0 истинный левый предикат сразу ведёт в тело if.
int GuardedDivisionOr(int divisor)
{
    if (divisor == 0 || 100 / divisor < 4)
    {
        return 1;
    }
    return 0;
}
