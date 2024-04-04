import re

def test():
    while True:
        expected = input()
        actual = input()
        test_case = input()

        if 'nan' in expected and 'nan' in actual:
            continue

        eparts = re.split('\\.|p', expected)
        aparts = re.split('\\.|p', actual)
        if '.' not in expected:
            if eparts[0].endswith('0x0') and eparts[0] == aparts[0] and eparts[1] == aparts[2] == '+0':
                continue
            if eparts[0].endswith('0x1') and aparts[0] == aparts[0] and aparts[1].ljust(6, '0') == '000000' and eparts[1] == aparts[2]:
                continue
            assert actual == expected, f'{actual} != {expected} {test_case}'
        else:
            assert eparts[0] == aparts[0], f'{actual} != {expected} {test_case}'
            assert eparts[2] == aparts[2], f'{actual} != {expected} {test_case}'
            assert eparts[1].ljust(6, '0') == aparts[1].ljust(6, '0'), f'{actual} != {expected} {test_case}'


try:
    test()
except EOFError:
    print("All print tests passed with no errors")
