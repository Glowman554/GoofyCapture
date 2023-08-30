const file = JSON.parse(Deno.readTextFileSync("build/compile_commands.json")) as [ {
	directory: string,
	command: string,
	file: string
} ];

const args: string[] = [];
for (const p of file) {
	for (const c of p.command.split(" ")) {
		if (c.startsWith("-")) {
			if (!args.includes(c)) {
				args.push(c);
			}
		}
	}
}
console.log("Found " + args.length + " arguments!");
Deno.writeTextFileSync("compile_flags.txt", args.join("\n"));
