// 38. Два последовательных цикла
// Цель: Выход первого цикла является входом второго.
// В графе: Два отдельных цикла CFG и перенос определения total между ними.
int TwoSequentialLoops(int n, int m)
{
    int total = 0;
    for (int i = 0; i < n; i++)
    {
        total = total + i;
    }
    for (int j = 0; j < m; j++)
    {
        total = total - j;
    }
    return total;
}
