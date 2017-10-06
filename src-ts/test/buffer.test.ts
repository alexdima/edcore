/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import * as assert from 'assert';
import { buildBufferFromFixture, readFixture } from './utils/bufferBuilder';
import { EdBuffer } from '../../index';
import { IOffsetLengthEdit, getRandomInt, generateEdits } from './utils';

const GENERATE_DELETE_TESTS = false;
const GENERATE_TESTS = false;
const PRINT_TIMES = false;
const ASSERT_INVARIANTS = true;

suite('Loading', () => {

    function assertBuffer(fileName: string): void {
        const buff = buildBufferFromFixture(fileName);
        const text = readFixture(fileName);

        assertAllMethods(buff, text);
        if (ASSERT_INVARIANTS) {
            buff.AssertInvariants();
        }
    }

    test('checker.txt', () => {
        assertBuffer('checker.txt');
    });

    test('checker-400.txt', () => {
        assertBuffer('checker-400.txt');
    });

    test('checker-10.txt', () => {
        assertBuffer('checker-10.txt');
    });
});

function applyOffsetLengthEdits(initialContent: string, edits: IOffsetLengthEdit[]): string {
    // TODO: ensure edits are sorted bottom up
    edits = edits.slice(0);

    edits.sort((e1, e2) => {
        return e1.offset - e2.offset;
    });
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

suite('DeleteOneOffsetLen', () => {

    interface IOffsetLengthDelete {
        offset: number;
        length: number;
    }

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
            const time = PRINT_TIMES ? process.hrtime() : null;
            buff.ReplaceOffsetLen([{
                offset: edits[i].offset,
                length: edits[i].length,
                text: ''
            }]);
            const diff = PRINT_TIMES ? process.hrtime(time) : null;
            if (PRINT_TIMES) {
                console.log(`DeleteOneOffsetLen took ${diff[0] * 1e9 + diff[1]} nanoseconds, i.e. ${(diff[0] * 1e9 + diff[1]) / 1e6} ms.`);
            }
            assertAllMethods(buff, expected);
            if (ASSERT_INVARIANTS) {
                buff.AssertInvariants();
            }
        }
    }

    function _tt(name: string, fileInfo: IFileInfo, edits: IOffsetLengthDelete[]): void {
        if (name.charAt(0) === '_') {
            test.only(name, () => {
                assertConsecutiveDeleteOneOffsetLen(fileInfo, edits);
            });
        } else {
            test(name, () => {
                assertConsecutiveDeleteOneOffsetLen(fileInfo, edits);
            });
        }
    }

    suite('checker-400.txt', () => {
        const FILE_INFO: IFileInfo = {
            fileName: 'checker-400.txt',
            chunkSize: 1000
        };

        function tt(name: string, edits: IOffsetLengthDelete[]): void {
            _tt(name, FILE_INFO, edits);
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
            _tt(name, FILE_INFO, edits);
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

        test('gen6 - invalidate nodes', () => {
            runTest(192, [{ "offset": 8062, "length": 13646 }, { "offset": 7469, "length": 1925 }]);
        });

        test('gen7', () => {
            runTest(53340, [
                { "offset": 807, "length": 22287 },
                { "offset": 278, "length": 109 },
                { "offset": 628, "length": 152 },
                { "offset": 348, "length": 271 },
                { "offset": 282, "length": 29 },
                { "offset": 282, "length": 6 }
            ]);
        });

        test('gen8', () => {
            runTest(19671, [
                { "offset": 3478, "length": 12195 },
                { "offset": 645, "length": 830 },
                { "offset": 1346, "length": 7120 },
                { "offset": 1572, "length": 419 },
                { "offset": 1449, "length": 918 },
                { "offset": 391, "length": 161 },
                { "offset": 28, "length": 516 },
                { "offset": 805, "length": 0 },
                { "offset": 1005, "length": 19 },
                { "offset": 969, "length": 23 },
                { "offset": 718, "length": 72 },
                { "offset": 811, "length": 29 },
                { "offset": 6, "length": 318 },
                { "offset": 303, "length": 44 },
                { "offset": 126, "length": 155 },
                { "offset": 68, "length": 107 },
                { "offset": 198, "length": 46 }
            ]);
        });

        test('gen9 - join bug', () => {
            runTest(4603, [{ "offset": 12682, "length": 7664 }]);
        });

        test('gen10 - join bug', () => {
            runTest(7255, [{ "offset": 2327, "length": 14103 }]);
        });

        test('gen11 - join bug', () => {
            runTest(43728, [{ offset: 16159, length: 3017 }]);
        });
    });

    (function () {
        const CONSECUTIVE_EDITS_CNT = 100;
        const MIN_CHUNK_SIZE = 10;
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
                this._editsCnt = getRandomInt(CONSECUTIVE_EDITS_CNT, CONSECUTIVE_EDITS_CNT);
                this._edits = [];
            }

            run(): void {
                // console.log(this._chunkSize);
                for (let i = 0; i < this._editsCnt; i++) {
                    let _edits = generateEdits(this._content, 1, 1);
                    if (_edits.length === 0) {
                        continue;
                    }
                    let _edit = _edits[0];
                    let edit: IOffsetLengthDelete = {
                        offset: _edit.offset,
                        length: _edit.length
                    };
                    // console.log(edit);
                    this._edits[i] = edit;

                    this._content = applyOffsetLengthEdits(this._content, [{ offset: edit.offset, length: edit.length, text: '' }]);
                    this._buff.DeleteOneOffsetLen(edit.offset, edit.length);
                    assertAllMethods(this._buff, this._content);
                    if (ASSERT_INVARIANTS) {
                        this._buff.AssertInvariants();
                    }
                }
            }

            toString(): void {
                console.log(`runTest(${this._chunkSize}, ${JSON.stringify(this._edits)});`);
            }
        }

        const GENERATE_CNT = GENERATE_DELETE_TESTS ? 100000 : -1;
        for (let i = GENERATE_CNT; i >= 0; i--) {
            console.log(`REMAINING... ${i}`);
            let test = new AutoTest();
            try {
                test.run();
            } catch (err) {
                console.log(err);
                console.log(test.toString());
                i = -1;
            }
        }
    })();
});

suite('InsertOneOffsetLen', () => {

    interface IOffsetLengthInsert {
        offset: number;
        text: string;
    }

    interface IFileInfo {
        fileName: string;
        chunkSize: number;
    }

    function assertConsecutiveInsertOneOffsetLen(fileInfo: IFileInfo, edits: IOffsetLengthInsert[]): void {
        const buff = buildBufferFromFixture(fileInfo.fileName, fileInfo.chunkSize);
        const initialContent = readFixture(fileInfo.fileName);

        let expected = initialContent;
        for (let i = 0; i < edits.length; i++) {
            expected = applyOffsetLengthEdits(expected, [{ offset: edits[i].offset, length: 0, text: edits[i].text }]);
            const time = PRINT_TIMES ? process.hrtime() : null;
            buff.InsertOneOffsetLen(edits[i].offset, edits[i].text);
            const diff = PRINT_TIMES ? process.hrtime(time) : null;
            if (PRINT_TIMES) {
                console.log(`InsertOneOffsetLen took ${diff[0] * 1e9 + diff[1]} nanoseconds, i.e. ${(diff[0] * 1e9 + diff[1]) / 1e6} ms.`);
            }
            assertAllMethods(buff, expected);
            if (ASSERT_INVARIANTS) {
                buff.AssertInvariants();
            }
        }
    }

    function _tt(name: string, fileInfo: IFileInfo, edits: IOffsetLengthInsert[]): void {
        if (name.charAt(0) === '_') {
            test.only(name, () => {
                assertConsecutiveInsertOneOffsetLen(fileInfo, edits);
            });
        } else {
            test(name, () => {
                assertConsecutiveInsertOneOffsetLen(fileInfo, edits);
            });
        }
    }

    suite('checker-400.txt', () => {
        const FILE_INFO: IFileInfo = {
            fileName: 'checker-400.txt',
            chunkSize: 1000
        };

        function tt(name: string, edits: IOffsetLengthInsert[]): void {
            _tt(name, FILE_INFO, edits);
        }

        tt('simple insert: first char', [{ offset: 0, text: 'a' }]);
        // tt('simple delete: first line without EOL', [{ offset: 0, length: 45 }]);
        // tt('simple delete: first line with EOL', [{ offset: 0, length: 46 }]);
        // tt('simple delete: second line without EOL', [{ offset: 46, length: 33 }]);
        // tt('simple delete: second line with EOL', [{ offset: 46, length: 34 }]);
        // tt('simple delete: first two lines without EOL', [{ offset: 0, length: 79 }]);
        // tt('simple delete: first two lines with EOL', [{ offset: 0, length: 80 }]);
        // tt('simple delete: first chunk - 1', [{ offset: 0, length: 999 }]);
        // tt('simple delete: first chunk', [{ offset: 0, length: 1000 }]);
        // tt('simple delete: first chunk + 1', [{ offset: 0, length: 1001 }]);
        // tt('simple delete: last line', [{ offset: 22754, length: 47 }]);
        // tt('simple delete: last line with preceding EOL', [{ offset: 22753, length: 48 }]);
        // tt('simple delete: entire file', [{ offset: 0, length: 22801 }]);
    });
});

suite('ReplaceOffsetLen', () => {

    interface IFileInfo {
        fileName: string;
        chunkSize: number;
    }

    function assertOffsetLenEdits(fileInfo: IFileInfo, edits: IOffsetLengthEdit[][]): void {
        const buff = buildBufferFromFixture(fileInfo.fileName, fileInfo.chunkSize);
        const initialContent = readFixture(fileInfo.fileName);

        let expected = initialContent;
        for (let i = 0; i < edits.length; i++) {
            const parallelEdits = edits[i];

            expected = applyOffsetLengthEdits(expected, parallelEdits);
            const time = PRINT_TIMES ? process.hrtime() : null;
            buff.ReplaceOffsetLen(parallelEdits);
            const diff = PRINT_TIMES ? process.hrtime(time) : null;
            if (PRINT_TIMES) {
                console.log(`ReplaceOffsetLen took ${diff[0] * 1e9 + diff[1]} nanoseconds, i.e. ${(diff[0] * 1e9 + diff[1]) / 1e6} ms.`);
            }
            if (ASSERT_INVARIANTS) {
                buff.AssertInvariants();
            }
            assertAllMethods(buff, expected);
        }
    }

    function _tt(name: string, fileInfo: IFileInfo, edits: IOffsetLengthEdit[][]): void {
        if (name.charAt(0) === '_') {
            test.only(name, () => {
                assertOffsetLenEdits(fileInfo, edits);
            });
        } else {
            test(name, () => {
                assertOffsetLenEdits(fileInfo, edits);
            });
        }
    }

    suite('checker-400.txt', () => {
        const FILE_INFO: IFileInfo = {
            fileName: 'checker-400.txt',
            chunkSize: 1000
        };

        function tt(name: string, edits: IOffsetLengthEdit[][]): void {
            _tt(name, FILE_INFO, edits);
        }

        tt('simple insert: first char', [
            [
                { offset: 1, length: 0, text: 'a' },
                { offset: 0, length: 0, text: 'a' },
            ]
        ]);
        // tt('simple delete: first line without EOL', [{ offset: 0, length: 45 }]);
        // tt('simple delete: first line with EOL', [{ offset: 0, length: 46 }]);
        // tt('simple delete: second line without EOL', [{ offset: 46, length: 33 }]);
        // tt('simple delete: second line with EOL', [{ offset: 46, length: 34 }]);
        // tt('simple delete: first two lines without EOL', [{ offset: 0, length: 79 }]);
        // tt('simple delete: first two lines with EOL', [{ offset: 0, length: 80 }]);
        // tt('simple delete: first chunk - 1', [{ offset: 0, length: 999 }]);
        // tt('simple delete: first chunk', [{ offset: 0, length: 1000 }]);
        // tt('simple delete: first chunk + 1', [{ offset: 0, length: 1001 }]);
        // tt('simple delete: last line', [{ offset: 22754, length: 47 }]);
        // tt('simple delete: last line with preceding EOL', [{ offset: 22753, length: 48 }]);
        // tt('simple delete: entire file', [{ offset: 0, length: 22801 }]);
    });

    suite('generated', () => {
        function runTest(fileName: string, chunkSize: number, edits: IOffsetLengthEdit[][]): void {
            assertOffsetLenEdits({
                fileName: fileName,
                chunkSize: chunkSize
            }, edits);
        }

        // test.only('gen1 - \\r\\n boundary case within chunk', () => {
        //     runTest('checker-400-CRLF.txt', 35315, [[{"offset":22641,"length":112,"text":"\ne\n"}]]);
        //     // runTest(59302, [{ "offset": 13501, "length": 2134 }]);
        // });

        test('auto1', () => {
            runTest("checker-10.txt", 44576, [[{ "offset": 177, "length": 17, "text": "\n" }]]);
        });

        test('auto2', () => {
            runTest("checker-10.txt", 29797, [[{ "offset": 11, "length": 27, "text": "u\n" }, { "offset": 44, "length": 31, "text": "x\nntd\nj" }]])
        });

        test('auto3', () => {
            runTest("checker-10.txt", 55940, [
                [
                    { offset: 1, length: 0, text: 'bgm\nme\n' },
                    { offset: 1, length: 0, text: '\n' },
                    { offset: 1, length: 82, text: '\npq' }
                ]
            ]);
        });

        test('auto4', () => {
            runTest("checker-10.txt", 5779, [
                [
                    { offset: 10, length: 2, text: 'bp\n' },
                    { offset: 17, length: 0, text: 'msf\nn' },
                    { offset: 18, length: 121, text: 'sp' }
                ]
            ]);
        });

        test('auto5', () => {
            runTest("checker-10.txt", 1949, [
                [
                    { "offset": 1, "length": 1, "text": "ab\nc\neo" }, 
                    { "offset": 3, "length": 4, "text": "uj" }, 
                    { "offset": 9, "length": 11, "text": "p\nepp" }, 
                    { "offset": 22, "length": 199, "text": "m" }
                ]
            ]);
        });

        // GENERATE_TESTS=false;
    });

    (function () {
        // const FILE_NAME = 'checker-400-CRLF.txt';
        const FILE_NAME = 'checker-400-CRLF.txt';
        const MIN_PARALLEL_EDITS_CNT = 1;
        const MAX_PARALLEL_EDITS_CNT = 10;
        const MIN_CONSECUTIVE_EDITS_CNT = 1;
        const MAX_CONSECUTIVE_EDITS_CNT = 1;
        const MIN_CHUNK_SIZE = 10;
        const MAX_CHUNK_SIZE = 1 << 16;

        class AutoTest {
            private _buff: EdBuffer;
            private _content: string;
            private _chunkSize: number;
            private _editsCnt: number;
            private _edits: IOffsetLengthEdit[][];

            constructor() {
                this._chunkSize = getRandomInt(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE);
                this._buff = buildBufferFromFixture(FILE_NAME, this._chunkSize);
                this._content = readFixture(FILE_NAME);
                this._editsCnt = getRandomInt(MIN_CONSECUTIVE_EDITS_CNT, MAX_CONSECUTIVE_EDITS_CNT);
                this._edits = [];
            }

            run(): void {
                // console.log(this._chunkSize);
                for (let i = 0; i < this._editsCnt; i++) {
                    let edits = generateEdits(this._content, MIN_PARALLEL_EDITS_CNT, MAX_PARALLEL_EDITS_CNT);
                    if (edits.length === 0) {
                        continue;
                    }
                    // console.log(_edits);
                    this._edits.push(edits);
                    this._content = applyOffsetLengthEdits(this._content, edits);
                    this._buff.ReplaceOffsetLen(edits);
                    assertAllMethods(this._buff, this._content);
                    if (ASSERT_INVARIANTS) {
                        this._buff.AssertInvariants();
                    }
                }
            }

            toString(): void {
                console.log(`runTest(${JSON.stringify(FILE_NAME)}, ${this._chunkSize}, ${JSON.stringify(this._edits)});`);
            }
        }

        const GENERATE_CNT = GENERATE_TESTS ? 10000 : -1;
        for (let i = GENERATE_CNT; i > 0; i--) {
            // if (global.gc) {
            //     // if (i % 100 === 0) {
            //         global.gc();
            //     // }
            // } else {
            //     console.log('Garbage collection unavailable.  Pass --expose-gc '
            //       + 'when launching node to enable forced garbage collection.');
            // }
            console.log(`REMAINING... ${i}`);
            let test = new AutoTest();
            try {
                test.run();
            } catch (err) {
                console.log(err);
                console.log(test.toString());
                i = -1;
            }
            test = null;
        }
    })();
});

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
