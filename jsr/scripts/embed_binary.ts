import { parseArgs } from "jsr:@std/cli@1.0.24";


const args = parseArgs<{
  binaryPath : string,
  target : string
}>(Deno.args);

const jsrBinPath = './jsr/bin';

Deno.mkdirSync(jsrBinPath, {recursive: true});

Deno.copyFileSync(args.binaryPath, `${jsrBinPath}/x84_64.so`);

const dataArray = Deno.readFileSync(args.binaryPath);

Deno.writeTextFileSync(
  `${jsrBinPath}/x84_64.json`,
  JSON.stringify({
    target: args.target,
    filename: args.binaryPath,
    encoding: 'base64',
    data: dataArray.toBase64()
  })
);

