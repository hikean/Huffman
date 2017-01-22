# -*- coding: utf-8 -*-
"""Huffman Algorithm Compressor

Usage:
    huffman.py encode [-vt] <in_file_name> <out_file_name>
    huffman.py decode [-vt] <in_file_name> <out_file_name>
    huffman.py --version
    huffman.py --help

Arguments:
    <in_file_name>  the input file name
    <out_file_name> the output file name

Options:
    -h --help       show this help message and exit
    --version       show version and exit
    -v --verbose    print huffman code table
    -t --tree       print huffman code tree

Examples:
    huffman.py encode -vt in.txt out.huff
    huffman.py decode -t in.huff out.txt

"""

import struct
import heapq
from docopt import docopt
from curses.ascii import isprint


class HuffmanNode(object):
    def __init__(self, v=None, l=None, r=None):
        self.value, self.left, self.right = v, l, r


class HuffmanCoder(object):
    def __init__(self, frequency_map):
        self.frequency_map = frequency_map
        self.byte_map, self.root = {}, None
        self.heap = [(frequency_map[key] * 655360 + key, HuffmanNode(key)) for key in range(0, 256)
                     if key in frequency_map]  # ] and key != ord('\r')]
        self.all_chars_count = sum([frequency_map[key] for key in frequency_map])
        self.unique_char_count = len(frequency_map)
        self.__build_tree()

    def __build_tree(self):
        heapq.heapify(self.heap)
        while len(self.heap) > 1:
            first = heapq.heappop(self.heap)
            second = heapq.heappop(self.heap)
            heapq.heappush(self.heap, (first[0] + second[0], HuffmanNode(None, first[1], second[1])))
        self.root = heapq.heappop(self.heap)[1]
        self.__init_byte_map(self.root)

    def __save_huffman_information(self, file_out):
        file_out.write(struct.pack("i", self.all_chars_count))
        file_out.write(struct.pack("i", self.unique_char_count))
        for i in range(0, 256):
            if i in self.frequency_map:
                file_out.write(struct.pack("B", i))
                file_out.write(struct.pack("i", self.frequency_map[i]))

    def __decode_byte_loop(self, buff, mask, current):
        ret = []
        while mask:
            if not current.left and not current.right:
                ret.append(current.value)
                current = self.root
            else:
                current = current.right if (mask & buff) else current.left
                mask /= 2
        return ret, current

    def __decode_byte(self, buff, mask, current):
        if not mask:
            return [], current
        if not current.left and not current.right:
            ret, node = self.__decode_byte(buff, mask, self.root)
            return [current.value] + ret, node
        elif (mask & buff) == 0:
            return self.__decode_byte(buff, mask / 2, current.left)
        else:
            return self.__decode_byte(buff, mask / 2, current.right)

    def __init_byte_map(self, current, prefix=""):
        if not current:
            return
        if not current.left and not current.right:
            self.byte_map[current.value] = prefix
        else:
            self.__init_byte_map(current.left, prefix + '0')
            self.__init_byte_map(current.right, prefix + '1')

    def decode_bytes(self, byte_in, out_file_name):
        ls, decoded_byte_count, current = [], 0, self.root
        file_out = open(out_file_name, "wb")
        if self.unique_char_count <= 1:
            for key in self.frequency_map:
                for i in range(self.all_chars_count):
                    file_out.write(struct.pack("B", key))
        else:
            for buff in byte_in:
                ls, current = self.__decode_byte_loop(ord(buff), 128, current)
                if not current:
                    print "[!] ERROR HAPPENED WHILE DECODING!"
                    return
                for b in ls:
                    if decoded_byte_count < self.all_chars_count:
                        file_out.write(struct.pack("B", b))
                        decoded_byte_count += 1
        file_out.close()

    def encode_fast(self, byte_in, out_file_name):
        byte_str = "".join([self.byte_map[ord(byte)] for byte in byte_in])
        byte_str += "0" * (8 - len(byte_str) % 8)
        byte_bin = [struct.pack("B", int(byte_str[i*8:i*8+8], base=2)) for i in range(len(byte_str)//8)]
        file_out = open(out_file_name, "wb")
        self.__save_huffman_information(file_out)
        file_out.write("".join(byte_bin))
        return len(byte_bin)

    def encode_bytes(self, byte_in, out_file_name):
        file_out = open(out_file_name, "wb")
        self.__save_huffman_information(file_out)
        mask, buff, write_byte_count = 128, 0, 0
        for byte in byte_in:
            # if byte == '\r':
            #     continue
            for bit in self.byte_map[ord(byte)]:
                if bit == '1':
                    buff |= mask
                mask /= 2
                if mask == 0:
                    file_out.write(struct.pack("B", buff))
                    mask, buff, write_byte_count = 128, 0, write_byte_count + 1
        if mask != 128:
            file_out.write(struct.pack("B", buff))
            write_byte_count += 1
        file_out.close()
        return write_byte_count

    def print_huffman_code(self):
        for key in self.byte_map:
            print "\'%c\'" % chr(key) if isprint(key) else "0x%02x" % key, self.byte_map[key]

    def __print_tree(self, root, indent=""):
        if not root:
            return []
        if not root.left and not root.right:
            return [indent + "|___" + ("\'%c\'" % root.value if isprint(root.value) else "0x%02x" % root.value) +
                    " %d" % self.frequency_map[root.value]]
        else:
            return [indent + "|___(*)"] + self.__print_tree(root.left, indent + "    ") + \
                   self.__print_tree(root.right, indent + "    ")

    def print_tree(self):
        lines = self.__print_tree(self.root)
        for index in range(len(lines)):
            fork_index = lines[index].find("|")
            if fork_index >= 0:
                for i in range(index - 1, 0, -1):
                    if lines[i][fork_index] == ' ':
                        lines[i] = lines[i][:fork_index] + "|" + lines[i][fork_index + 1:]
                    else:
                        break
        for line in lines:
            print line


def huffman_decode(in_file_name, out_file_name, verbose=False, tree=False):
    with open(in_file_name, "rb") as file_in:
        all_chars_count, unique_char_count = 0, 0
        bytes_head = file_in.read(8)
        if len(bytes_head) == 0:
            print "Empty File"
            open(out_file_name, "w")
            return
        elif len(bytes_head) == 8:
            all_chars_count, unique_char_count = struct.unpack("ii", bytes_head)
        else:
            print "Broken File"
            exit(-1)
        # print all_chars_count, unique_char_count
        if unique_char_count > 255:
            print "[!] Input File %s Format Error" % out_file_name
            return
        frequency_map = {}
        for i in range(unique_char_count):
            char = struct.unpack("B", file_in.read(1))[0]
            frequency_map[char] = struct.unpack("i", file_in.read(4))[0]
        coder = HuffmanCoder(frequency_map)
        coder.decode_bytes(file_in.read(), out_file_name)
        if verbose:
            coder.print_huffman_code()
        if tree:
            coder.print_tree()


def huffman_encode(in_file_name, out_file_name, verbose=False, tree=False):
    with open(in_file_name, "rb") as file_in:
        chars = file_in.read()
    frequency_map = {}
    if len(chars) == 0:
        print "Empty File, No Need to Compress"
        open(out_file_name, "w")
        return
    for char in chars:
        char = ord(char)
        if char not in frequency_map:
            frequency_map[char] = 1
        else:
            frequency_map[char] += 1
    coder = HuffmanCoder(frequency_map)
    write_byte_count = coder.encode_fast(chars, out_file_name)
    # coder.encode_fast has the completely same function as coder.encode_bytes
    # write_byte_count = coder.encode_bytes(chars, out_file_name)
    if verbose:
        coder.print_huffman_code()
    if tree:
        coder.print_tree()
    print "Origin Size:", coder.all_chars_count
    print "Encoding Size:", write_byte_count
    print "Huffman File Size:", write_byte_count + 8 + 5 * coder.unique_char_count
    print "Compress Rate:", 0 if not coder.all_chars_count else \
        write_byte_count / (coder.all_chars_count * 1.0)


if __name__ == "__main__":
    arguments = docopt(__doc__, version='V0.0.1')
    in_file, out_file = arguments["<in_file_name>"], arguments["<out_file_name>"]
    verbose_flag, tree_flag = arguments["--verbose"], arguments["--tree"]
    if arguments["encode"]:
        print "\nEncoding", in_file, "to", out_file
        huffman_encode(in_file, out_file, verbose_flag, tree_flag)
    else:
        print "\nDecoding", in_file, "to", out_file
        huffman_decode(in_file, out_file, verbose_flag, tree_flag)
