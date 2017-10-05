/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

export declare class EdBuffer {
    _nativeEdBufferBrand: void;
    constructor();

    AssertInvariants(): void;

    GetLength(): number;
    GetLineCount(): number;
    GetLineContent(lineNumber: number): string;
    DeleteOneOffsetLen(offset: number, length: number): void;
    InsertOneOffsetLen(offset: number, text: string): void;
}

export declare class EdBufferBuilder {
    _nativeEdBufferBuilderBrand: void;

    constructor();

    AcceptChunk(chunk: string): void;
    Finish(): string;
    Build(): EdBuffer;
}
