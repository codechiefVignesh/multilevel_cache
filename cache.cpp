// CS22B2004 VIGNESH ARAVINDH B
// CS22B2053 DHIVYA DHARSHAN V

#include <bits/stdc++.h>

using namespace std;

#define L1_SIZE 2048 // 2K words
#define L1_BLOCK_SIZE 16
#define L1_NUM_BLOCKS (L1_SIZE / L1_BLOCK_SIZE)

#define L2_SIZE 16384 // 16K words
#define L2_WAYS 4
#define L2_NUM_SETS (L2_SIZE / (L2_WAYS * L1_BLOCK_SIZE))

#define MAIN_MEM_SIZE 65536 // 64K words
#define BUFFER_SIZE 4 

struct cache
{
    int tag, last_access_time;
    bool valid, dirty;
    cache() : tag(-1), valid(false), dirty(false), last_access_time(0){}
};

class victimcache
{
    deque<cache> blocks;
public:
    victimcache() { blocks.resize(BUFFER_SIZE); }

    void insert(cache c)
    {
        // Avoid duplicate insertions
        for (auto &block : blocks) {
            if (block.tag == c.tag) {
                return; // Already in victim cache
            }
        }
        if (blocks.size() >= BUFFER_SIZE)
        {
            blocks.pop_front(); // Evict oldest block
        }
        blocks.push_back(c);
    }

    bool access(int tag)
    {
        for (auto &block : blocks)
        {
            if (block.tag == tag)
            {
                return true;
            }
        }
        return false;
    }
};

class l1cache
{
    vector<cache> blocks;
public:
    l1cache() { blocks.resize(L1_NUM_BLOCKS); }

    bool access(int address, victimcache &victim)
    {
        int index = (address / L1_BLOCK_SIZE) % L1_NUM_BLOCKS;
        int tag = address / L1_SIZE;

        if (blocks[index].valid && blocks[index].tag == tag)
        {
            return true; // L1 Hit
        }

        // Before replacing, check if the current block exists in the victim cache
        if (blocks[index].valid)
        {
            victim.insert(blocks[index]); // Move evicted L1 block to victim cache
        }

        blocks[index].tag = tag;
        blocks[index].valid = true;
        return false; // L1 Miss
    }
};

class l2cache
{
    vector<vector<cache>> sets;

public:
    l2cache() { sets.resize(L2_NUM_SETS, vector<cache>(L2_WAYS)); }

    int lru(vector<cache> &search)
    {
        int min = INT_MAX;
        int min_idx = 0;
        for (int i = 0; i < search.size(); i++)
        {
            if (min > search[i].last_access_time)
            {
                min = search[i].last_access_time;
                min_idx = i;
            }
        }
        return min_idx;
    }

    bool access(int address)
    {
        int index = (address / L1_BLOCK_SIZE) % L2_NUM_SETS;
        int tag = address / L2_SIZE;

        for (auto &block : sets[index])
        {
            if (block.valid && block.tag == tag)
            {
                block.last_access_time += 1;
                return true; // L2 Hit
            }
        }

        for (auto &block : sets[index])
        {
            if (!block.valid)
            {
                block.tag = tag;
                block.valid = true;
                return false;
            }
        }

        // Apply LRU Replacement Policy
        int replacement_block_idx = lru(sets[index]);
        sets[index][replacement_block_idx].tag = tag;
        sets[index][replacement_block_idx].valid = true;
        return false;
    }
};

class PrefetchBuffer
{
    queue<int> buffer;

public:
    void prefetch(int addr)
    {
        if (buffer.size() >= BUFFER_SIZE)
            buffer.pop();
        buffer.push(addr);
    }

    bool access(int addr)
    {
        queue<int> temp = buffer;
        while (!temp.empty())
        {
            if (temp.front() == addr)
                return true;
            temp.pop();
        }
        return false;
    }
};

class MultilevelCache
{
    l1cache l1;
    l2cache l2;
    victimcache victim;
    PrefetchBuffer instr_pre, data_pre;
    int l1_hits = 0;
    int l1_miss = 0;
    int l2_miss = 0;
    int l2_hits = 0;
    int victim_hit = 0;
    int victim_miss = 0;

public:
    void access_memory(int addr, bool isInstruction)
    {
        std::ofstream log("cache_log.txt", std::ios::app);
    
    log << addr << " ";  // Log the address

    if (l1.access(addr, victim))
    {
        l1_hits++;
        log << "L1_HIT\n";
    }
    else
    {
        l1_miss++;
        log << "L1_MISS ";
        
        if (victim.access(addr))
        {
            victim_hit++;
            log << "VC_HIT\n";
        }
        else
        {
            victim_miss++;
            log << "VC_MISS ";
            
            if (l2.access(addr))
            {
                l2_hits++;
                log << "L2_HIT\n";
            }
            else
            {
                l2_miss++;
                log << "L2_MISS MEM_ACCESS\n";
            }
        }
    }

        // Prefetch next block
        int next_block = addr + L1_BLOCK_SIZE;
        if (isInstruction)
            instr_pre.prefetch(next_block);
        else
            data_pre.prefetch(next_block);
    }

    void display_cache_status()
    {
        std::ofstream outfile("cache_stats.txt");  // Open file for writing
    
        std::cout << "L1 Hits: " << l1_hits << std::endl;
        std::cout << "L1 Misses: " << l1_miss << std::endl;
        std::cout << "L2 Hits: " << l2_hits << std::endl;
        std::cout << "L2 Misses: " << l2_miss << std::endl;
        std::cout << "Victim Misses: " << victim_miss << std::endl;
        std::cout << "Victim Hits: " << victim_hit << std::endl;
    
        float l1_miss_rate = (float)l1_miss / (l1_hits + l1_miss);
        float l2_miss_rate = (l1_miss > 0) ? (float)l2_miss / l1_miss : 0;
        float effective_miss_rate = l1_miss_rate * l2_miss_rate;
    
        std::cout << "Effective Miss Rate in one-level cache: " << l1_miss_rate << std::endl;
        std::cout << "Effective Miss Rate in two-level cache: " << effective_miss_rate << std::endl;
    
        // Writing to file
        outfile << "L1 Hits: " << l1_hits << std::endl;
        outfile << "L1 Misses: " << l1_miss << std::endl;
        outfile << "L2 Hits: " << l2_hits << std::endl;
        outfile << "L2 Misses: " << l2_miss << std::endl;
        outfile << "Victim Misses: " << victim_miss << std::endl;
        outfile << "Victim Hits: " << victim_hit << std::endl;
        outfile << "Effective Miss Rate in one-level cache: " << l1_miss_rate << std::endl;
        outfile << "Effective Miss Rate in two-level cache: " << effective_miss_rate << std::endl;
    
        outfile.close();  // Close the file
    }
};

int main()
{
    MultilevelCache mutlicache;
    ifstream infile("addresses.txt");
    int address;

    const int SIZE = 30000;
    vector<bool> random_flags(SIZE);

    srand(time(0)); // Seed for randomness

    for (int i = 0; i < SIZE; i++) 
    {
        random_flags[i] = rand() % 2;
    }
    int idx = 0;
    while (infile >> address)
    {
        mutlicache.access_memory(address, random_flags[idx++]);
    }
    mutlicache.display_cache_status();
    return 0;
}
