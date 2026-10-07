// 40. Ветвление после цикла
// Цель: Результат цикла определяет последующее ветвление.
// В графе: Предикат после цикла читает значение total со слияния выходных путей цикла.
int AdjustAfterLoop(int n)
{
    int total = 0;
    for (int i = 0; i < n; i++)
    {
        total = total + i;
    }
    if (total > 10)
    {
        total = total - 10;
    }
    return total;
}
