// 49. Побочный эффект в условии цикла
// Цель: Короткое замыкание определяет, произойдёт ли присваивание в условии.
// В графе: При ложном count < limit remaining не меняется; иначе меняется даже при ложном втором предикате.
int ConditionSideEffect(int limit, int budget)
{
    int count = 0;
    int remaining = budget;
    while (count < limit && (remaining = remaining - 1) > 0)
    {
        count++;
    }
    return count * 100 + remaining;
}
