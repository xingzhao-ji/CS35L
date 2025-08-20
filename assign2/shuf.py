#!/usr/bin/python
import argparse
import sys
import random
import string

def main():
    parser = argparse.ArgumentParser(prog='shuf')
    parser.add_argument('-e', '--echo', action='store_true', help='treat args as input lines')
    parser.add_argument('-i', '--input-range', metavar='LO-HI', help='treat numbers from LO to HI as input lines')
    parser.add_argument('-n', '--head-count', type=int, help='output at most COUNT lines')
    parser.add_argument('-r', '--repeat', action='store_true', help='allow repeats; without -n runs forever')
    parser.add_argument('inputs', nargs='*', help='operands for echo mode or input filename')
    args = parser.parse_args()

    echo_option = args.echo
    input_range_option = args.input_range is not None
    head_count_option = args.head_count is not None
    repeat_option = args.repeat

    if echo_option and input_range_option:
        parser.error('cannot combine --echo and --input-range')
    if echo_option:
        if not args.inputs:
            parser.error('no arguments for --echo')
    elif input_range_option:
        if args.inputs:
            parser.error('--input-range takes no operands')
    else:
        if len(args.inputs) > 1:
            parser.error('too many operands')

    if echo_option:
        buffer = list(args.inputs)
    elif input_range_option:
        try:
            lo_str, hi_str = args.input_range.split('-', 1)
            lo, hi = int(lo_str), int(hi_str)
        except Exception:
            parser.error('invalid input range')
        if lo > hi or lo < 0:
            parser.error('invalid input range')
        buffer = [str(i) for i in range(lo, hi + 1)]
    else:
        filename = args.inputs[0] if args.inputs else '-'
        try:
            if filename == '-':
                text = sys.stdin.read()
            else:
                with open(filename, 'r') as f:
                    text = f.read()
        except IOError:
            print('shuf: error: cannot open', filename, file=sys.stderr)
            sys.exit(1)
        buffer = text.splitlines()

    random.shuffle(buffer)

    if repeat_option and head_count_option:
        for _ in range(args.head_count):
            print(random.choice(buffer))
    elif repeat_option:
        while True:
            print(random.choice(buffer))
    elif head_count_option:
        if args.head_count > len(buffer):
            parser.error('head-count greater than number of lines')
        for line in buffer[:args.head_count]:
            print(line)
    else:
        for line in buffer:
            print(line)

if __name__ == '__main__':
    main()
