// 12. Вложенный if
// Цель: Внутренний if существует только в истинной внешней ветви.
// В графе: Вложенная зависимость по управлению и слияния на двух уровнях.
int ClassifyRange(int x)
{
    int result;
    if (x > 0)
    {
        if (x > 10)
        {
            result = 2;
        }
        else
        {
            result = 1;
        }
    }
    else
    {
        result = 0;
    }
    return result;
}
