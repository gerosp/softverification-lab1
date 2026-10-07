// 08. Две ветви if/else
// Цель: Две взаимоисключающие ветви определяют одну переменную.
// В графе: Оба пути сходятся перед return; для SSA требуется слияние result.
int AbsoluteValue(int x)
{
    int result;
    if (x < 0)
    {
        result = -x;
    }
    else
    {
        result = x;
    }
    return result;
}
