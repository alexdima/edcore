/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/


export interface IOffsetLengthEdit {
    offset: number;
    length: number;
    text: string;
}

export function getRandomInt(min: number, max: number): number {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

const _a = 'a'.charCodeAt(0);
const _z = 'z'.charCodeAt(0);

function getRandomString(minLength: number, maxLength: number): string {
    let length = getRandomInt(minLength, maxLength);
    let r = '';
    for (let i = 0; i < length; i++) {
        r += String.fromCharCode(getRandomInt(_a, _z));
    }
    return r;
}

function getRandomEOL(): string {
    switch (getRandomInt(1, 3)) {
        case 1: return '\r';
        case 2: return '\n';
        case 3: return '\r\n';
    }
}

function generateFile(length: number): string {
    let result = '';
    while (result.length < length) {
        let line = getRandomString(0, Math.min(100, length - result.length))
        if (result.length + line.length >= length) {
            result += line;
        } else {
            result += line + getRandomEOL();
        }
    }
    return result;
}

export const enum EditType { Regular, Inserts, Deletes };

export function generateEdits(editType: EditType, content: string, minCnt: number, maxCnt: number): IOffsetLengthEdit[] {

    let result: IOffsetLengthEdit[] = [];
    let cnt = getRandomInt(minCnt, maxCnt);

    let maxOffset = content.length;

    while (cnt > 0 && maxOffset > 0) {

        let offset = getRandomInt(0, maxOffset);
        let length = getRandomInt(0, maxOffset - offset);
        let textLength = (
            editType === EditType.Inserts
                ? Math.round(0.50 * maxOffset)
                : editType === EditType.Deletes
                    ? Math.round(0.01 * maxOffset)
                    : Math.round(0.10 * maxOffset)
        );
        let text = generateFile(textLength);

        result.push({
            offset: offset,
            length: length,
            text: text
        });

        maxOffset = offset;
        cnt--;
    }

    result.reverse();

    return result;
}
