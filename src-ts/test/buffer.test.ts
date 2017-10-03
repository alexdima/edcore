/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import * as assert from 'assert';
import { buildBufferFromFixture, readFixture } from './utils/bufferBuilder';
import { EdBuffer } from '../../index';
import { IOffsetLengthEdit, getRandomInt, generateEdits } from './utils';

suite('Loading', () => {

    test('checker.txt', () => {
        assertBuffer('checker.txt');
    });
});

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

interface IOffsetLengthDelete {
    offset: number;
    length: number;
}

suite('DeleteOneOffsetLen', () => {

    interface IFileInfo {
        fileName: string;
        chunkSize: number;
    }

    function assertConsecutiveDeleteOneOffsetLen(fileInfo: IFileInfo, edits: IOffsetLengthDelete[]): void {
        const buff = buildBufferFromFixture(fileInfo.fileName, fileInfo.chunkSize);
        const initialContent = readFixture(fileInfo.fileName);

        let expected = initialContent;
        for (let i = 0; i < edits.length; i++) {
            expected = applyOffsetLengthEdits(expected, [{ offset: edits[i].offset, length: edits[i].length, text: '' }]);
            const time = process.hrtime();
            buff.DeleteOneOffsetLen(edits[i].offset, edits[i].length);
            const diff = process.hrtime(time);
            console.log(`DeleteOneOffsetLen took ${diff[0] * 1e9 + diff[1]} nanoseconds, i.e. ${(diff[0] * 1e9 + diff[1]) / 1e6} ms.`);
            assertAllMethods(buff, expected);
        }
    }

    suite('checker-400.txt', () => {
        const FILE_INFO: IFileInfo = {
            fileName: 'checker-400.txt',
            chunkSize: 1000
        };

        function tt(name: string, edits: IOffsetLengthDelete[]): void {
            test(name, () => {
                assertConsecutiveDeleteOneOffsetLen(FILE_INFO, edits);
            });
        }

        tt('simple delete: first char', [{ offset: 0, length: 1 }]);
        tt('simple delete: first line without EOL', [{ offset: 0, length: 45 }]);
        tt('simple delete: first line with EOL', [{ offset: 0, length: 46 }]);
        tt('simple delete: second line without EOL', [{ offset: 46, length: 33 }]);
        tt('simple delete: second line with EOL', [{ offset: 46, length: 34 }]);
        tt('simple delete: first two lines without EOL', [{ offset: 0, length: 79 }]);
        tt('simple delete: first two lines with EOL', [{ offset: 0, length: 80 }]);
        tt('simple delete: first chunk - 1', [{ offset: 0, length: 999 }]);
        tt('simple delete: first chunk', [{ offset: 0, length: 1000 }]);
        tt('simple delete: first chunk + 1', [{ offset: 0, length: 1001 }]);
        tt('simple delete: last line', [{ offset: 22754, length: 47 }]);
        tt('simple delete: last line with preceding EOL', [{ offset: 22753, length: 48 }]);
        tt('simple delete: entire file', [{ offset: 0, length: 22801 }]);
    });

    suite('checker-400-CRLF.txt', () => {
        const FILE_INFO: IFileInfo = {
            fileName: 'checker-400-CRLF.txt',
            chunkSize: 1000
        };

        function tt(name: string, edits: IOffsetLengthDelete[]): void {
            test(name, () => {
                assertConsecutiveDeleteOneOffsetLen(FILE_INFO, edits);
            });
        }

        tt('simple delete: first char', [{ offset: 0, length: 1 }]);
        tt('simple delete: first line without EOL', [{ offset: 0, length: 45 }]);
        tt('simple delete: first line with CR', [{ offset: 0, length: 46 }]);
        tt('simple delete: first line with EOL', [{ offset: 0, length: 47 }]);
        tt('simple delete: second line without EOL', [{ offset: 47, length: 33 }]);
        tt('simple delete: second line with CR', [{ offset: 47, length: 34 }]);
        tt('simple delete: second line with EOL', [{ offset: 47, length: 35 }]);
        tt('simple delete: first two lines without EOL', [{ offset: 0, length: 80 }]);
        tt('simple delete: first two lines with CR', [{ offset: 0, length: 81 }]);
        tt('simple delete: first two lines with EOL', [{ offset: 0, length: 82 }]);
        tt('simple delete: first chunk - 1', [{ offset: 0, length: 999 }]);
        tt('simple delete: first chunk', [{ offset: 0, length: 1000 }]);
        tt('simple delete: first chunk + 1', [{ offset: 0, length: 1001 }]);
        tt('simple delete: last line', [{ offset: 23152, length: 47 }]);
        tt('simple delete: last line with preceding LF', [{ offset: 23151, length: 48 }]);
        tt('simple delete: last line with preceding EOL', [{ offset: 23150, length: 49 }]);
        tt('simple delete: entire file', [{ offset: 0, length: 23199 }]);
    });

    suite('generated', () => {
        function runTest(chunkSize: number, edits: IOffsetLengthDelete[]): void {
            assertConsecutiveDeleteOneOffsetLen({
                fileName: 'checker-400-CRLF.txt',
                chunkSize: chunkSize
            }, edits);
        }

        test('gen1 - \\r\\n boundary case within chunk', () => {
            runTest(59302, [{ "offset": 13501, "length": 2134 }]);
        });

        test('gen2 - endless loop', () => {
            runTest(36561, [{ "offset": 23199, "length": 0 }]);
        });

        test('gen3 - \\r\\n boundary case outisde chunk 1', () => {
            runTest(20646, [{ "offset": 19478, "length": 1287 }]);
        });

        test('gen4 - \\r\\n boundary case outisde chunk 2', () => {
            runTest(2195, [{ "offset": 12512, "length": 2249 }]);
        });

        test('gen5 - \\r\\n boundary case outisde chunk 3', () => {
            runTest(201, [{ "offset": 19720, "length": 2203 }]);
        });

        test('gen6', () => {
            runTest(192, [{ "offset": 8062, "length": 13646 }, { "offset": 7469, "length": 1925 }]);
        })
    });
});

(function () {
    const CONSECUTIVE_EDITS_CNT = 2;
    const MIN_CHUNK_SIZE = 100;
    const MAX_CHUNK_SIZE = 1 << 16;

    class AutoTest {
        private _buff: EdBuffer;
        private _content: string;
        private _chunkSize: number;
        private _editsCnt: number;
        private _edits: IOffsetLengthDelete[];

        constructor() {
            this._chunkSize = getRandomInt(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE);
            this._buff = buildBufferFromFixture('checker-400-CRLF.txt', this._chunkSize);
            this._content = readFixture('checker-400-CRLF.txt');
            this._editsCnt = getRandomInt(1, CONSECUTIVE_EDITS_CNT);
            this._edits = [];
        }

        run(): void {
            console.log(this._chunkSize);
            for (let i = 0; i < this._editsCnt; i++) {
                let _edit = generateEdits(this._content, 1, 1)[0];
                let edit: IOffsetLengthDelete = {
                    offset: _edit.offset,
                    length: _edit.length
                };
                console.log(edit);
                this._edits[i] = edit;

                this._content = applyOffsetLengthEdits(this._content, [{ offset: edit.offset, length: edit.length, text: '' }]);
                this._buff.DeleteOneOffsetLen(edit.offset, edit.length);
                assertAllMethods(this._buff, this._content);
            }
        }

        toString(): void {
            console.log(`runTest(${this._chunkSize}, ${JSON.stringify(this._edits)});`);
        }
    }

    const GENERATE_CNT = -1;//100000;
    for (let i = GENERATE_CNT; i >= 0; i--) {
        console.log(`REMAINING... ${i}`);
        let test = new AutoTest();
        try {
            test.run();
        } catch (err) {
            console.log(test.toString());
            i = -1;
        }
    }
})();

function assertBuffer(fileName: string): void {
    const buff = buildBufferFromFixture(fileName);
    const text = readFixture(fileName);

    assertAllMethods(buff, text);
}

function assertAllMethods(buff: EdBuffer, text: string): void {
    assert.equal(buff.GetLength(), text.length, 'length');

    const lines = constructLines(text);
    assert.equal(buff.GetLineCount(), lines.length, 'line count');

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
