with open("template.txt") as f:
    template = f.read()

with open("tables.tex", "w") as out:
    with open("suca.txt") as f:
        while True:
            caption = next(f).strip()
            label = next(f).strip()
            data = ""
            try:
                while len(line := next(f).strip()) > 0:
                    data = data + line + "\n"
            except StopIteration:
                pass
            data = data.strip()
        
            out.write(template
                .replace("CAPTION", caption)
                .replace("LABEL", label)
                .replace("DATA", data)
            )
            out.write("\n")