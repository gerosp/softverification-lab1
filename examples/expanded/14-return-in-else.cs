// 14. Return в обеих ветвях
// Цель: Явный else после ветви, завершающей метод.
// В графе: Обе ветви заканчиваются выходом; продолжения после if нет.
int ReturnInElse(int x)
{
    if (x < 0)
    {
        return -1;
    }
    else
    {
        int result = x + 1;
        return result;
    }
}
