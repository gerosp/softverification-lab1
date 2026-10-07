// 22. Базовый do/while
// Цель: Тело выполняется до первой проверки условия.
// В графе: При n <= 0 тело всё равно выполняется один раз; вход ведёт прямо в тело.
int CountDoWhile(int n)
{
    int count = 0;
    do
    {
        count++;
    }
    while (count < n);
    return count;
}
