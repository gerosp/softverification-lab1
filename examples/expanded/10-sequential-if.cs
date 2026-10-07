// 10. Последовательные if
// Цель: Два if могут сработать независимо друг от друга.
// В графе: После первого слияния второй предикат проверяется на обоих путях.
int IndependentConditions(int x)
{
    int result = 0;
    if (x >= 0)
    {
        result = result + 1;
    }
    if (x % 2 == 0)
    {
        result = result + 2;
    }
    return result;
}
