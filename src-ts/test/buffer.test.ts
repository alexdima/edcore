/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import * as assert from 'assert';
import { buildBufferFromFixture, readFixture } from './utils/bufferBuilder';
import { EdBuffer } from '../../index';

suite('Loading', () => {

    test('checker.txt', () => {
        assertBuffer('checker.txt');
    });
});

interface IOffsetLengthEdit {
    offset: number;
    length: number;
    text: string;
}

function applyOffsetLengthEdits(initialContent: string, edits: IOffsetLengthEdit[]): string {
    // TODO: ensure edits are sorted bottom up
    let result = initialContent;
    for (let i = edits.length - 1; i >= 0; i--) {
        result = (
            result.substring(0, edits[i].offset) +
            edits[i].text +
            result.substring(edits[i].offset + edits[i].length)
        );
    }
    return result;
}

suite('Editing: DeleteOneOffsetLen', () => {

    function assertDeleteOneOffsetLen(fileName: string, offset: number, length: number): void {
        const buff = buildBufferFromFixture(fileName);
        const initialContent = readFixture(fileName);
        const expected = applyOffsetLengthEdits(initialContent, [{ offset: offset, length: length, text: '' }]);
        
        buff.DeleteOneOffsetLen(offset, length);
        assertAllMethods(buff, expected);
    }

    test('simple delete', () => {
        assertDeleteOneOffsetLen('checker.txt', 0, 1);
    });
});

function assertBuffer(fileName: string): void {
    const buff = buildBufferFromFixture(fileName);
    const text = readFixture(fileName);

    assertAllMethods(buff, text);
}

function assertAllMethods(buff: EdBuffer, text: string): void {
    assert.equal(buff.GetLength(), text.length);
    
    const lines = constructLines(text);
    assert.equal(buff.GetLineCount(), lines.length);

    for (let i = 0; i < lines.length; i++) {
        let actual = buff.GetLineContent(i + 1);
        let expected = lines[i];
        assert.equal(actual, expected, '@ line number ' + (i + 1));
    }
}

/**
 * Split a string into lines, taking \r\n, \n or \r as possible line terminators.
 */
function constructLines(text: string): string[] {
    const len = text.length;

    let result: string[] = [], resultLen = 0;
    let lineStartOffset = 0;
    let prevChCode = 0;

    for (let i = 0; i < len; i++) {
        const chCode = text.charCodeAt(i);

        if (chCode === 10 /* \n */) {
            result[resultLen++] = text.substring(lineStartOffset, i + 1);
            lineStartOffset = i + 1;
        } else if (prevChCode === 13 /* \r */) {
            result[resultLen++] = text.substring(lineStartOffset, i);
            lineStartOffset = i;
        }

        prevChCode = chCode;
    }

    result[resultLen++] = text.substring(lineStartOffset, len + 1);
    if (prevChCode === 13) {
        result[resultLen++] = '';
    }

    return result;
}
