
declare module "../build/Debug/edcore" {
    export class EdBuffer {
        _nativeEdBufferBrand: void;
        constructor();

        GetLineCount(): number;
        GetLineContent(lineNumber: number): string;
    }

    export class EdBufferBuilder {
        _nativeEdBufferBuilderBrand: void;

        constructor();

        AcceptChunk(chunk: string): void;
        Finish(): string;
        Build(): EdBuffer;
    }
}
