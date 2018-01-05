/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

export interface IOffsetLenEdit {
    offset: number;
    length: number;
    text: string;
}

export declare class EdBuffer {
    _nativeEdBufferBrand: void;
    constructor();

    AssertInvariants(): void;

    GetLength(): number;
    GetLineCount(): number;
    GetOffsetAt(lineNumber: number, column: number): number;
    GetLineContent(lineNumber: number): string;
    ReplaceOffsetLen(edits: IOffsetLenEdit[]): void;
}

export declare class EdBufferBuilder {
    _nativeEdBufferBuilderBrand: void;

    constructor();

    AcceptChunk(chunk: string): void;
    Finish(): string;
    Build(): EdBuffer;
}
