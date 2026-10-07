// 19. Switch с default
// Цель: Многовариантный выбор; здесь break завершает switch.
// В графе: Все case и default ведут к общему продолжению; break направлен за switch.
int SelectCode(int code)
{
    int result;
    switch (code)
    {
        case 0:
            result = 0;
            break;
        case 1:
            result = 10;
            break;
        case 2:
            result = 20;
            break;
        default:
            result = -1;
            break;
    }
    return result;
}
