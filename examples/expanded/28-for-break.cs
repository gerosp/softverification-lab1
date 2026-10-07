// 28. Break в одном for
// Цель: Досрочный выход из for до обновления аккумулятора.
// В графе: break пропускает шаг i++ и условие for; начальный и накопленный total достигают return.
int SumWithForBreak(int n, int limit)
{
    int total = 0;
    for (int i = 0; i < n; i++)
    {
        if (i >= limit)
        {
            break;
        }
        total = total + i;
    }
    return total;
}
