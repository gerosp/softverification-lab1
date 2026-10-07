// 09. Пустая ветвь else
// Цель: Ветка else содержит пустой блок.
// В графе: Пустой блок не должен обрывать ложный путь к return.
int EmptyElse(bool enabled)
{
    int result = 10;
    if (enabled)
    {
        result = result + 5;
    }
    else
    {
    }
    return result;
}
