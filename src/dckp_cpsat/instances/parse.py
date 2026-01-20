import re
import dataclasses
import typing

class InvalidInstanceException(Exception):
    def __init__(self, regex, line):
        super().__init__(self, f'Could not match "{regex}" against "{line}"')

@dataclasses.dataclass
class InstanceParams:
    n: int
    c: int

@dataclasses.dataclass
class InstanceItem:
    i: int
    p: int
    w: int
    
@dataclasses.dataclass
class InstanceConflict:
    i: int
    j: int

REGEXES = {
    k: re.compile(v) for k,v in {
        "n": r"^param n := (\d+);$",
        "c": r"^param c := (\d+);?$",
        "vertices_hdr": r"^param : V : p w :=$",
        "conflicts_hdr": r"^set E :=$",
        "vertex": r"^\s+(\d+)\s+(\d+)\s+(\d+)$",
        "conflict": r"^\s+(\d+)\s+(\d+)$",
        "semicolon": r"^;$",
        "empty": r"^$",
    }.items()
}

class InstanceParser:
    def __init__(self, path):
        self.path = path
        self.state = -1
    
    def __parse_line(self, regex):
        line = self.file.readline()
        match = regex.match(line)
        if match is None:
            raise InvalidInstanceException(regex.pattern, line)
        return match
    
    def __parse_list(self, entry_regex, sent_regex):
        while True:
            line = self.file.readline()
            sent_match = sent_regex.match(line)
            if sent_match is not None:
                break
            entry_match = entry_regex.match(line)
            if entry_match is None:
                raise InvalidInstanceException(entry_regex.pattern, line)
            yield entry_match
            
    def __enter__(self):
        if self.state != -1:
            raise RuntimeError("Invalid state.")
    
        self.file = open(self.path, 'r', encoding='utf-8')
        self.state = 0
        return self
        
    def __exit__(self, *args):
        self.file.close()
    
    def read_parameters(self) -> InstanceParams:
        if self.state != 0:
            raise RuntimeError("Invalid state.")
        self.state = 1
        
        ans = InstanceParams(
            n = int(self.__parse_line(REGEXES["n"]).group(1)),
            c = int(self.__parse_line(REGEXES["c"]).group(1))
        )
        
        self.state = 2
        
        return ans
    
    def read_items(self)  -> typing.Iterable[InstanceItem]:
        if self.state != 2:
            raise RuntimeError("Invalid state.")
        self.state = 3
        
        self.__parse_line(REGEXES["vertices_hdr"])
        
        yield from (
            InstanceItem(i=int(x.group(1)), p=int(x.group(2)), w=int(x.group(3)))
            for x in self.__parse_list(REGEXES["vertex"], REGEXES["semicolon"])   
        )
        
        self.__parse_line(REGEXES["empty"])
        self.state = 4
        
    def read_conflicts(self) -> typing.Iterable[InstanceConflict]:
        if self.state != 4:
            raise RuntimeError("Invalid state.")
        
        self.state = 5
        
        self.__parse_line(REGEXES["conflicts_hdr"])
        
        yield from (
            InstanceConflict(i=int(x.group(1)), j=int(x.group(2)))
            for x in self.__parse_list(REGEXES["conflict"], REGEXES["semicolon"])
        )
        
        for line in self.file:
            if REGEXES["empty"].match(line) is None:
                raise InvalidInstanceException(REGEXES["empty"].pattern, line)
        
        self.state = 6