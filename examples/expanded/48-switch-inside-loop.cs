// 48. Switch внутри цикла
// Цель: Различение области действия break у switch и цикла.
// В графе: Каждый break завершает switch, после чего выполняется i++; цикл не прерывается.
int SumSwitchInLoop(int n)
{
    int total = 0;
    for (int i = 0; i < n; i++)
    {
        switch (i % 3)
        {
            case 0:
                total = total + 1;
                break;
            case 1:
                total = total + 2;
                break;
            default:
                total = total + 3;
                break;
        }
    }
    return total;
}
