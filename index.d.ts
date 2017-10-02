/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

export declare class EdBuffer {
    _nativeEdBufferBrand: void;
    constructor();

    GetLineCount(): number;
    GetLineContent(lineNumber: number): string;
}

export declare class EdBufferBuilder {
    _nativeEdBufferBuilderBrand: void;

    constructor();

    AcceptChunk(chunk: string): void;
    Finish(): string;
    Build(): EdBuffer;
}
