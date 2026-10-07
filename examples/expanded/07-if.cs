// 07. Одиночный if
// Цель: Одна условная ветвь, явного else нет.
// В графе: Ложная ветвь обходит присваивание; перед return возможны два определения result.
int PositiveOrZero(int x)
{
    int result = 0;
    if (x > 0)
    {
        result = x;
    }
    return result;
}
