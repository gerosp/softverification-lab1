// 36. Continue перед break
// Цель: Сочетание двух уже разобранных переходов в одном цикле.
// В графе: continue обходит проверку break; цели переходов различны: шаг и выход цикла.
int SumWithSkipAndStop(int n, int limit)
{
    int total = 0;
    for (int i = 0; i < n; i++)
    {
        if (i % 2 == 0)
        {
            continue;
        }
        if (i >= limit)
        {
            break;
        }
        total = total + i;
    }
    return total;
}
