// Tree-sitter counts UTF-8 bytes; Monaco/JavaScript count UTF-16 code units.
export function byteToUtf16(source: string, byteOffset: number): number {
  let bytes = 0;
  let units = 0;
  for (const char of source) {
    const cp = char.codePointAt(0)!;
    const width = cp <= 0x7f ? 1 : cp <= 0x7ff ? 2 : cp <= 0xffff ? 3 : 4;
    if (bytes + width > byteOffset) break;
    bytes += width;
    units += char.length;
  }
  return units;
}
