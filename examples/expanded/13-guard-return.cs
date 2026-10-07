// 13. Ранний return
// Цель: Проверка завершает метод до основного вычисления.
// В графе: Ранний return ведёт к выходу метода и не к следующему присваиванию.
int GuardedDouble(int value, bool enabled)
{
    if (!enabled)
    {
        return 0;
    }
    int result = value * 2;
    return result;
}
