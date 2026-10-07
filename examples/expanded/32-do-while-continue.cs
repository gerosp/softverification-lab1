// 32. Continue в одном do/while
// Цель: Пропуск остатка тела с переходом к нижнему условию.
// В графе: continue ведёт к условию do/while, а не прямо к началу тела.
int SumWithDoContinue(int n)
{
    int i = 0;
    int total = 0;
    do
    {
        i++;
        if (i % 2 == 0)
        {
            continue;
        }
        total = total + i;
    }
    while (i < n);
    return total;
}
