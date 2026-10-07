// 33. Return из одного цикла
// Цель: Найденный делитель немедленно завершает метод.
// В графе: return из тела направлен к выходу метода, минуя завершающий return -1.
int FirstDivisor(int n)
{
    for (int divisor = 2; divisor < n; divisor++)
    {
        if (n % divisor == 0)
        {
            return divisor;
        }
    }
    return -1;
}
