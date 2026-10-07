// 30. Continue в одном for
// Цель: Continue пропускает суммирование чётных значений.
// В графе: Цель continue — шаг i++, после которого проверяется условие for.
int SumWithForContinue(int n)
{
    int total = 0;
    for (int i = 0; i < n; i++)
    {
        if (i % 2 == 0)
        {
            continue;
        }
        total = total + i;
    }
    return total;
}
