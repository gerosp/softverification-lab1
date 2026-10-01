int CountPairs(int n)
{
    int total = 0;
    for (int i = 0; i < n; i++)
    {
        int j = 0;
        while (j < n)
        {
            j++;
            if (j == i)
                continue;
            if (j > i + 2)
                break;
            total++;
        }
    }
    return total;
}
