#include <iostream>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <map>
#include <queue>
#include <sstream>
#include <vector>

using std::string;
#define  CHAR_COUNT 256
#define  NODE_END -1

int frequency[CHAR_COUNT] = {0}, unique_char_count = 0, all_chars_count = 0, huffman_byte_count = 0;

struct HuffNode {
    unsigned char value;
    int frequency, left, right;
    
    HuffNode(unsigned char v = 0, int f = 0, int l = NODE_END, int r = NODE_END) :
            value(v), frequency(f), left(l), right(r)
    {}
    
    bool operator<(const HuffNode &arg) const
    {
        if (this->frequency != arg.frequency)
            return this->frequency > arg.frequency;
        return this->value > arg.value;
    }
} tree_nodes[CHAR_COUNT << 1];

void init_frequency(FILE *file)
{
    memset(frequency, 0, sizeof(frequency));
    unsigned char tmp;
    while (fread(&tmp, sizeof(unsigned char), 1, file) == 1)
    {
        if (frequency[tmp] == 0)
            unique_char_count++;
        frequency[tmp]++;
        all_chars_count++;
    }
}

int build_huffman_tree(int *frequency, HuffNode *tree_nodes)
{
    std::priority_queue<HuffNode> que;
    int root = 0;
    for (int i = 0; i < CHAR_COUNT; i++)
        if (frequency[i] > 0)
            que.push(HuffNode((unsigned char) i, frequency[i]));
    while (!que.empty())
    {
        auto &left = tree_nodes[root++] = que.top();
        que.pop();
        if (que.empty())
            return root - 1;
        auto &right = tree_nodes[root++] = que.top();
        que.pop();
        que.push(HuffNode(std::max(left.value, right.value), left.frequency + right.frequency, root - 2, root - 1));
    }
    return root - 1;
}

string get_char_string(unsigned char chr)
{
    char tmp[10] = {0};
    tmp[0] = char(chr);
    if (chr == '\r')
        sprintf(tmp, "\\r");
    else if (chr == '\n')
        sprintf(tmp, "\\n");
    else if (chr == '\t')
        sprintf(tmp, "\\t");
    else if (chr & 128 || !isprint(chr))
        sprintf(tmp, "0x%x", chr);
    return string(tmp);
}

void get_tree_lines(std::vector<string> &lines, HuffNode *nodes, int root, int indent = 0)
{
    if (root == NODE_END)
        return;
    HuffNode now = nodes[root];
    std::stringstream ss;
    if (indent >= 4)
        ss << string(indent - 3, ' ') << "|____(" << now.frequency << ")";
    else
        ss << "\n-+(" << now.frequency << ")";
    if (now.left == NODE_END && now.right == NODE_END)
    {
        ss << "'" << get_char_string(now.value) << "'";
        lines.push_back(ss.str());
    }
    else
    {
        lines.push_back(ss.str());
        get_tree_lines(lines, nodes, now.left, indent + 4);
        get_tree_lines(lines, nodes, now.right, indent + 4);
    }
}

void print_tree(HuffNode *nodes, int root)
{
    std::vector<string> lines;
    get_tree_lines(lines, nodes, root);
    const int line_count = int(lines.size());
    for (int i = line_count - 1; i > 0; i--)
        for (int j = int(lines[i].size()) - 1; j >= 0; j--)
            if (lines[i][j] == '|' && lines[i - 1][j] == ' ')
                for (int t = i - 1; t >= 0 && lines[t][j] == ' '; t--)
                    lines[t][j] = '|';
    for (int i = 0; i < line_count; i++)
        std::cout << lines[i] << std::endl;
}

void create_map(std::map<unsigned char, string> &mp, HuffNode *nodes, int root, string prefix)
{
    if (root == NODE_END)
        return;
    HuffNode now = nodes[root];
    if (now.left == NODE_END && now.right == NODE_END)
    {
        mp[now.value] = prefix;
        std::cout << "'" << get_char_string(now.value) << "' : " << prefix << std::endl;
    }
    create_map(mp, nodes, now.left, prefix + "0");
    create_map(mp, nodes, now.right, prefix + "1");
}

void read_huffman_information(FILE *file)
{
    memset(frequency, 0, sizeof(frequency));
    unsigned char tmp;
    fread(&all_chars_count, sizeof(int), 1, file);
    fread(&unique_char_count, sizeof(int), 1, file);
    for (int i = 0, count; i < unique_char_count && i < CHAR_COUNT; i++)
    {
        fread(&tmp, sizeof(unsigned char), 1, file);
        fread(&count, sizeof(int), 1, file);
        frequency[tmp] = count;
    }
}

void save_frequency_information(FILE *file)
{
    fwrite(&all_chars_count, sizeof(int), 1, file);
    fwrite(&unique_char_count, sizeof(int), 1, file);
    for (int i = 0; i < CHAR_COUNT; i++)
        if (frequency[i] > 0)
        {
            unsigned char tmp = (unsigned char) i;
            fwrite(&tmp, sizeof(unsigned char), 1, file);
            fwrite(&frequency[i], sizeof(int), 1, file);
        }
}

void huffman_encode(const char *in_file_name, const char *out_file_name)
{
    FILE *file_in = fopen(in_file_name, "rb");
    if (file_in == NULL)
    {
        printf("Error: File %s Open Failed\n", in_file_name);
        exit(-1);
    }
    init_frequency(file_in);
    rewind(file_in);
    FILE *file_out = fopen(out_file_name, "wb");
    const int root = build_huffman_tree(frequency, tree_nodes);
    print_tree(tree_nodes, root);
    save_frequency_information(file_out);
    std::map<unsigned char, string> char2code;
    create_map(char2code, tree_nodes, root, "");
    unsigned char tmp, mask = 128, buff = 0;
    while (fread(&tmp, sizeof(unsigned char), 1, file_in) == 1)
        for (auto x : char2code[tmp])
        {
            if (x == '1')
                buff |= mask;
            else
                buff &= ~mask;
            mask >>= 1;
            if (!mask)
            {
                huffman_byte_count += fwrite(&buff, sizeof(unsigned char), 1, file_out);
                mask = 128;
            }
        }
    if (mask != 128)
        huffman_byte_count += fwrite(&buff, sizeof(unsigned char), 1, file_out);
    fclose(file_in);
    fclose(file_out);
}

bool tree_decode(FILE *file, int root, unsigned char &buff, unsigned char &mask, unsigned char &value)
{
    if (root == NODE_END)
        return false;
    const HuffNode &now = tree_nodes[root];
    if (now.left == NODE_END && now.right == NODE_END)
    {
        value = now.value;
        return true;
    }
    else if (mask)
    {
        int bit = buff & mask;
        mask >>= 1;
        if (!mask && fread(&buff, sizeof(unsigned char), 1, file) == 1)
            mask = 128;
        if (bit == 0)
            return tree_decode(file, now.left, buff, mask, value);
        else
            return tree_decode(file, now.right, buff, mask, value);
    }
    return false;
}

void huffman_decode(const char *in_file_name, const char *out_file_name)
{
    unsigned char value = 0, buff = 0, mask = 128;
    FILE *file_in = fopen(in_file_name, "rb");
    if (file_in == NULL)
    {
        printf("Error: File %s Open Failed\n", in_file_name);
        exit(-1);
    }
    read_huffman_information(file_in);
    FILE *file_out = fopen(out_file_name, "wb");
    if (unique_char_count == 1)
    {
        for (int i = 0; i < CHAR_COUNT; i++)
            if (frequency[i] > 0)
                value = (unsigned char) i;
        for (int i = 0; i < all_chars_count; i++)
            fwrite(&value, sizeof(unsigned char), 1, file_out);
    }
    else if (all_chars_count != 0)
    {
        const int root = build_huffman_tree(frequency, tree_nodes);
        fread(&buff, sizeof(unsigned char), 1, file_in);
        print_tree(tree_nodes, root);
        for (int i = 0; i < all_chars_count; i++)
            if (tree_decode(file_in, root, buff, mask, value))
                fwrite(&value, sizeof(unsigned char), 1, file_out);
            else
            {
                puts("Decode Error");
                exit(-1);
            }
    }
    fclose(file_in);
    fclose(file_out);
}

void usage()
{
    puts("Usage:");
    puts("    ./huff encode in.txt out.huff");
    puts("    ./huff decode in.huff out.txt");
}

int main(int argc, char **args)
{
    if (argc < 4)
        usage();
    else if (args[1][0] == 'e')
    {
        puts("Encoding...");
        huffman_encode(args[2], args[3]);
        printf("Origin Size: %d Byte\nEncoding Size: %d Byte\nHuffman File Size: %d Byte\n",
               all_chars_count, huffman_byte_count, huffman_byte_count + 8 + 5 * unique_char_count);
        printf("Compress Rate(Encoding/Origin): %.6lf\n", huffman_byte_count * 1.0 / all_chars_count);
    }
    else
    {
        puts("Decoding...");
        huffman_decode(args[2], args[3]);
    }
    return 0;
}
