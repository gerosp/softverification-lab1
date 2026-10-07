// 31. Continue в одном while
// Цель: Счётчик увеличивается перед возможным continue.
// В графе: continue возвращает к условию while; увеличение счётчика уже произошло.
int SumWithWhileContinue(int n)
{
    int i = 0;
    int total = 0;
    while (i < n)
    {
        i++;
        if (i % 2 == 0)
        {
            continue;
        }
        total = total + i;
    }
    return total;
}
