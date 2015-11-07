import sys
import collections

def read_syscalls():
    out = collections.OrderedDict()

    for line in sys.stdin:
        if not line or line[0] == '#' or line[0] == '\n':
            continue

        if line[-1] == '\n':
            line = line[0:-1]

        sline = line.split(' ')

        if len(sline) != 2:
            raise RuntimeError("invalid line: " + line)

        id = sline[0]
        name = sline[1]

        out[id] = name

    return out

def generate_enum(data):
    for id, name in data.items():
        print(name + ' = ' + id + ',')

def generate_register(data):
    for id, name in data.items():
        print("registerHandler(" + name + ", hndl::" + name + ");")

def generate_glibc_defines(data):
    for id, name in data.items():
        print("#define __NR_" + name + " " + id)

generators = {
        "enum" : generate_enum,
        "register" : generate_register,
        "glibc_defines" : generate_glibc_defines
        }

def main(argv):
    if len(argv) != 2:
        sys.stderr.write("Specify a generator: " + ", ".join(generators.keys()) + "\n")
        sys.exit(1)

    generator = generators.get(argv[1])

    if generator is None:
        sys.stderr.write("Invalid generator, valid values: " +
                ", ".join(generators.keys()) + "\n")
        sys.exit(1)

    data = read_syscalls()
    generator(data)

if __name__ == '__main__':
    main(sys.argv)
