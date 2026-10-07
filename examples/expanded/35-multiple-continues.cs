// 35. Две точки continue
// Цель: Два независимых основания пропустить остаток итерации.
// В графе: Оба continue ведут к одному шагу for; второй предикат не проверяется на первом пути.
int SumWithTwoContinues(int n)
{
    int total = 0;
    for (int i = 0; i < n; i++)
    {
        if (i % 2 == 0)
        {
            continue;
        }
        if (i % 3 == 0)
        {
            continue;
        }
        total = total + i;
    }
    return total;
}
