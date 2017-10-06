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

function generateFile(small: boolean): string {
    let lineCount = getRandomInt(1, small ? 3 : 10);
    let lines: string[] = [];
    for (let i = 0; i < lineCount; i++) {
        lines.push(getRandomString(0, small ? 3 : 10) + getRandomEOL());
    }
    return lines.join('');
}

export function generateEdits(content: string, min: number, max: number): IOffsetLengthEdit[] {

    let result: IOffsetLengthEdit[] = [];
    let cnt = getRandomInt(min, max);

    let maxOffset = content.length;

    while (cnt > 0 && maxOffset > 0) {

        let offset = getRandomInt(0, maxOffset);
        let length = getRandomInt(0, maxOffset - offset);
        let text = generateFile(true);

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
