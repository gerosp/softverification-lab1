// 04. Независимые вычисления
// Цель: Две независимые цепочки объединяются в одном выражении.
// В графе: right не зависит от left, хотя его оператор расположен позже в CFG.
int CombineValues(int x, int y)
{
    int left = x + 1;
    int right = y - 1;
    int result = left + right;
    return result;
}
