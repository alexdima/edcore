/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import * as assert from 'assert';
import { buildBufferFromFixture, readFixture, buildBufferFromString } from './utils/bufferBuilder';
import { EdBuffer } from '../../index';
import { IOffsetLengthEdit, getRandomInt, generateEdits, EditType } from './utils';

const GENERATE_TESTS = false;
const PRINT_TIMES = true;
const ASSERT_INVARIANTS = true;

// const FILE_NAME = 'checker.txt';
const FILE_NAME = 'checker-400.txt';
const MIN_PARALLEL_EDITS_CNT = 1;
const MAX_PARALLEL_EDITS_CNT = 1;
const MIN_CONSECUTIVE_EDITS_CNT = 1;
const MAX_CONSECUTIVE_EDITS_CNT = 10;
const MIN_CHUNK_SIZE = 10;
const MAX_CHUNK_SIZE = 1 << 16;
const EDIT_TYPES = EditType.Inserts;

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

suite('ReplaceOffsetLen', () => {

    function applyOffsetLengthEdits(initialContent: string, edits: IOffsetLengthEdit[]): string {
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

    function _assertOffsetLenEdits(buff: EdBuffer, initialContent: string, edits: IOffsetLengthEdit[][]): void {
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

    function assertFixtureOffsetLenEdits(fileName: string, chunkSize: number, edits: IOffsetLengthEdit[][]): void {
        const buff = buildBufferFromFixture(fileName, chunkSize);
        const initialContent = readFixture(fileName);
        _assertOffsetLenEdits(buff, initialContent, edits);
    }

    function assertCustomOffsetLenEdits(initialContent: string, edits: IOffsetLengthEdit[][]): void {
        const buff = buildBufferFromString(initialContent);
        _assertOffsetLenEdits(buff, initialContent, edits);
    }

    function tt(name: string, fileName: string, chunkSize: number, edits: IOffsetLengthEdit[]): void {
        if (name.charAt(0) === '_') {
            test.only(name, () => {
                assertFixtureOffsetLenEdits(fileName, chunkSize, [edits]);
            });
        } else {
            test(name, () => {
                assertFixtureOffsetLenEdits(fileName, chunkSize, [edits]);
            });
        }
    }

    suite('checker-400.txt', () => {
        tt('simple delete: first char                  ', 'checker-400.txt', 1000, [{ offset: 0, length: 1, text: '' }]);
        tt('simple delete: first line without EOL      ', 'checker-400.txt', 1000, [{ offset: 0, length: 45, text: '' }]);
        tt('simple delete: first line with EOL         ', 'checker-400.txt', 1000, [{ offset: 0, length: 46, text: '' }]);
        tt('simple delete: second line without EOL     ', 'checker-400.txt', 1000, [{ offset: 46, length: 33, text: '' }]);
        tt('simple delete: second line with EOL        ', 'checker-400.txt', 1000, [{ offset: 46, length: 34, text: '' }]);
        tt('simple delete: first two lines without EOL ', 'checker-400.txt', 1000, [{ offset: 0, length: 79, text: '' }]);
        tt('simple delete: first two lines with EOL    ', 'checker-400.txt', 1000, [{ offset: 0, length: 80, text: '' }]);
        tt('simple delete: first chunk - 1             ', 'checker-400.txt', 1000, [{ offset: 0, length: 999, text: '' }]);
        tt('simple delete: first chunk                 ', 'checker-400.txt', 1000, [{ offset: 0, length: 1000, text: '' }]);
        tt('simple delete: first chunk + 1             ', 'checker-400.txt', 1000, [{ offset: 0, length: 1001, text: '' }]);
        tt('simple delete: last line                   ', 'checker-400.txt', 1000, [{ offset: 22754, length: 47, text: '' }]);
        tt('simple delete: last line with preceding EOL', 'checker-400.txt', 1000, [{ offset: 22753, length: 48, text: '' }]);
        tt('simple delete: entire file                 ', 'checker-400.txt', 1000, [{ offset: 0, length: 22801, text: '' }]);
        tt('simple insert: first char                  ', 'checker-400.txt', 1000, [{ offset: 0, length: 0, text: 'a' }]);
        tt('two inserts                                ', 'checker-400.txt', 1000, [{ offset: 1, length: 0, text: 'a' }, { offset: 0, length: 0, text: 'a' }]);
    });

    suite('checker-400-CRLF.txt', () => {
        tt('simple delete: first char                  ', 'checker-400-CRLF.txt', 1000, [{ offset: 0, length: 1, text: '' }]);
        tt('simple delete: first line without EOL      ', 'checker-400-CRLF.txt', 1000, [{ offset: 0, length: 45, text: '' }]);
        tt('simple delete: first line with CR          ', 'checker-400-CRLF.txt', 1000, [{ offset: 0, length: 46, text: '' }]);
        tt('simple delete: first line with EOL         ', 'checker-400-CRLF.txt', 1000, [{ offset: 0, length: 47, text: '' }]);
        tt('simple delete: second line without EOL     ', 'checker-400-CRLF.txt', 1000, [{ offset: 47, length: 33, text: '' }]);
        tt('simple delete: second line with CR         ', 'checker-400-CRLF.txt', 1000, [{ offset: 47, length: 34, text: '' }]);
        tt('simple delete: second line with EOL        ', 'checker-400-CRLF.txt', 1000, [{ offset: 47, length: 35, text: '' }]);
        tt('simple delete: first two lines without EOL ', 'checker-400-CRLF.txt', 1000, [{ offset: 0, length: 80, text: '' }]);
        tt('simple delete: first two lines with CR     ', 'checker-400-CRLF.txt', 1000, [{ offset: 0, length: 81, text: '' }]);
        tt('simple delete: first two lines with EOL    ', 'checker-400-CRLF.txt', 1000, [{ offset: 0, length: 82, text: '' }]);
        tt('simple delete: first chunk - 1             ', 'checker-400-CRLF.txt', 1000, [{ offset: 0, length: 999, text: '' }]);
        tt('simple delete: first chunk                 ', 'checker-400-CRLF.txt', 1000, [{ offset: 0, length: 1000, text: '' }]);
        tt('simple delete: first chunk + 1             ', 'checker-400-CRLF.txt', 1000, [{ offset: 0, length: 1001, text: '' }]);
        tt('simple delete: last line                   ', 'checker-400-CRLF.txt', 1000, [{ offset: 23152, length: 47, text: '' }]);
        tt('simple delete: last line with preceding LF ', 'checker-400-CRLF.txt', 1000, [{ offset: 23151, length: 48, text: '' }]);
        tt('simple delete: last line with preceding EOL', 'checker-400-CRLF.txt', 1000, [{ offset: 23150, length: 49, text: '' }]);
        tt('simple delete: entire file                 ', 'checker-400-CRLF.txt', 1000, [{ offset: 0, length: 23199, text: '' }]);
    });

    suite('custom', () => {
        test('simple insert', () => {
            assertCustomOffsetLenEdits('abc', [[{ offset: 1, length: 0, text: 'x' }]]);
        });
        test('simple delete', () => {
            assertCustomOffsetLenEdits('abc', [[{ offset: 1, length: 1, text: '' }]]);
        });
        test('simple replace', () => {
            assertCustomOffsetLenEdits('abc', [[{ offset: 1, length: 1, text: 'x' }]]);
        });
        test('generated 1', () => {
            assertCustomOffsetLenEdits("\n\nhtz", [[
                { "offset": 2, "length": 0, "text": "\n" },
                { "offset": 2, "length": 3, "text": "xrz" }
            ]]);
        });
        test('generated 1 inverse', () => {
            assertCustomOffsetLenEdits("\n\n\nxrz", [[
                { "offset": 2, "length": 1, "text": "" },
                { "offset": 3, "length": 3, "text": "htz" }
            ]]);
        });
        test('generated 2', () => {
            assertCustomOffsetLenEdits(
                "cwtkf\nn\nk\njkpkfwamq\nic\nexvwbbn\njulh\nk\nveyiig",
                [[
                    { "offset": 0, "length": 1, "text": "\ne\n" },
                    { "offset": 1, "length": 3, "text": "trf\nl" },
                    { "offset": 6, "length": 7, "text": "" },
                    { "offset": 15, "length": 0, "text": "" },
                    { "offset": 15, "length": 9, "text": "qqq" }
                ]]
            );
        });
        test('generated 3', () => {
            assertCustomOffsetLenEdits(
                "xxxxxx\n",
                [[
                    { "offset": 6, "length": 1, "text": "ecxa\rk\r" }
                ]]
            );
        });
        test('generated 4', () => {
            assertCustomOffsetLenEdits(
                "\n",
                [[
                    { "offset": 1, "length": 0, "text": "\nh\rsg\r\n" }
                ]]
            );
        });
        test('generated 5 - replacing CR', () => {
            assertCustomOffsetLenEdits(
                "lk\r\n",
                [[
                    { "offset": 2, "length": 1, "text": "qqe\r" }
                ]]
            );
        });
        test('generated 6 - adding LF after CR', () => {
            assertCustomOffsetLenEdits(
                "mzv\r",
                [[
                    { "offset": 4, "length": 0, "text": "\n" }
                ]]
            );
        });
        test('generated 7', () => {
            assertCustomOffsetLenEdits(
                "\nhy\n",
                [[
                    { "offset": 2, "length": 1, "text": "p\r" }
                ]]
            );
        });
        test('generated 8', () => {
            assertCustomOffsetLenEdits(
                "swc\r\n",
                [[
                    { "offset": 4, "length": 0, "text": "d\r\n\r" }
                ]]
            );
        });
        test('generated 9', () => {
            assertCustomOffsetLenEdits(
                "\r",
                [[
                    { "offset": 0, "length": 1, "text": "t\r" }
                ]]
            );
        });
        test('generated 10', () => {
            assertCustomOffsetLenEdits(
                "x\r\nw\r",
                [[
                    { "offset": 2, "length": 2, "text": "" }
                ]]
            );
        });
        test('generated 11', () => {
            assertCustomOffsetLenEdits(
                "\r\n",
                [[
                    { "offset": 1, "length": 1, "text": "oi\r\ng\r\n\r" }
                ]]
            );
        });
        test('generated 12', () => {
            assertCustomOffsetLenEdits(
                "bda\rj\r\r\n",
                [[
                    { "offset": 3, "length": 4, "text": "\r" }
                ]]
            );
        });
        test('generated 13', () => {
            assertCustomOffsetLenEdits(
                "ph\rdg\n\n",
                [[
                    { "offset": 2, "length": 3, "text": "\r" }
                ]]
            );
        });
        test('generated 14', () => {
            assertCustomOffsetLenEdits("tz\ro\r\n",
                [[
                    { "offset": 2, "length": 4, "text": "\r\r\n\n" }
                ]]
            );
        });
        test('generated 15', () => {
            assertCustomOffsetLenEdits(
                "\r\nje\n",
                [[
                    { "offset": 1, "length": 2, "text": "\n" }
                ]]
            );
        });
        test('generated 16', () => {
            assertCustomOffsetLenEdits(
                "grx\r\n",
                [[
                    { "offset": 4, "length": 1, "text": "lf_" }
                ]]
            );
        });
    });

    suite('speed', () => {
        test.only('copy-paste checker.txt', () => {
            const buff = buildBufferFromFixture('checker.txt');
            const initialContent = readFixture('checker.txt');

            const time = PRINT_TIMES ? process.hrtime() : null;
            buff.ReplaceOffsetLen([{
                offset: buff.GetLength(),
                length: 0,
                text: initialContent
            }]);
            const diff = PRINT_TIMES ? process.hrtime(time) : null;
            if (PRINT_TIMES) {
                console.log(`ReplaceOffsetLen took ${diff[0] * 1e9 + diff[1]} nanoseconds, i.e. ${(diff[0] * 1e9 + diff[1]) / 1e6} ms.`);
            }
        });
    });

    suite('generated', () => {
        const runTest = assertFixtureOffsetLenEdits;

        test('gen1 - \\r\\n boundary case within chunk', () => {
            runTest('checker-400-CRLF.txt', 59302, [[{ offset: 13501, length: 2134, text: '' }]]);
        });

        test('gen2 - endless loop', () => {
            runTest('checker-400-CRLF.txt', 36561, [[{ offset: 23199, length: 0, text: '' }]]);
        });

        test('gen3 - \\r\\n boundary case outisde chunk 1', () => {
            runTest('checker-400-CRLF.txt', 20646, [[{ offset: 19478, length: 1287, text: '' }]]);
        });

        test('gen4 - \\r\\n boundary case outisde chunk 2', () => {
            runTest('checker-400-CRLF.txt', 2195, [[{ offset: 12512, length: 2249, text: '' }]]);
        });

        test('gen5 - \\r\\n boundary case outisde chunk 3', () => {
            runTest('checker-400-CRLF.txt', 201, [[{ offset: 19720, length: 2203, text: '' }]]);
        });

        test('gen6 - invalidate nodes', () => {
            runTest('checker-400-CRLF.txt', 192, [
                [{ offset: 8062, length: 13646, text: '' }],
                [{ offset: 7469, length: 1925, text: '' }]
            ]);
        });

        test('gen7', () => {
            runTest('checker-400-CRLF.txt', 53340, [
                [{ offset: 807, length: 22287, text: '' }],
                [{ offset: 278, length: 109, text: '' }],
                [{ offset: 628, length: 152, text: '' }],
                [{ offset: 348, length: 271, text: '' }],
                [{ offset: 282, length: 29, text: '' }],
                [{ offset: 282, length: 6, text: '' }]
            ]);
        });

        test('gen8', () => {
            runTest('checker-400-CRLF.txt', 19671, [
                [{ offset: 3478, length: 12195, text: '' }],
                [{ offset: 645, length: 830, text: '' }],
                [{ offset: 1346, length: 7120, text: '' }],
                [{ offset: 1572, length: 419, text: '' }],
                [{ offset: 1449, length: 918, text: '' }],
                [{ offset: 391, length: 161, text: '' }],
                [{ offset: 28, length: 516, text: '' }],
                [{ offset: 805, length: 0, text: '' }],
                [{ offset: 1005, length: 19, text: '' }],
                [{ offset: 969, length: 23, text: '' }],
                [{ offset: 718, length: 72, text: '' }],
                [{ offset: 811, length: 29, text: '' }],
                [{ offset: 6, length: 318, text: '' }],
                [{ offset: 303, length: 44, text: '' }],
                [{ offset: 126, length: 155, text: '' }],
                [{ offset: 68, length: 107, text: '' }],
                [{ offset: 198, length: 46, text: '' }]
            ]);
        });

        test('gen9 - join bug', () => {
            runTest('checker-400-CRLF.txt', 4603, [[{ offset: 12682, length: 7664, text: '' }]]);
        });

        test('gen10 - join bug', () => {
            runTest('checker-400-CRLF.txt', 7255, [[{ offset: 2327, length: 14103, text: '' }]]);
        });

        test('gen11 - join bug', () => {
            runTest('checker-400-CRLF.txt', 43728, [[{ offset: 16159, length: 3017, text: '' }]]);
        });

        test('gen12', () => {
            runTest("checker-10.txt", 44576, [[{ "offset": 177, "length": 17, "text": "\n" }]]);
        });

        test('gen13', () => {
            runTest("checker-10.txt", 29797, [[{ "offset": 11, "length": 27, "text": "u\n" }, { "offset": 44, "length": 31, "text": "x\nntd\nj" }]])
        });

        test('gen14', () => {
            runTest("checker-10.txt", 55940, [
                [
                    { offset: 1, length: 0, text: 'bgm\nme\n' },
                    { offset: 1, length: 0, text: '\n' },
                    { offset: 1, length: 82, text: '\npq' }
                ]
            ]);
        });

        test('gen15', () => {
            runTest("checker-10.txt", 5779, [
                [
                    { offset: 10, length: 2, text: 'bp\n' },
                    { offset: 17, length: 0, text: 'msf\nn' },
                    { offset: 18, length: 121, text: 'sp' }
                ]
            ]);
        });

        test('gen16', () => {
            runTest("checker-10.txt", 1949, [
                [
                    { "offset": 1, "length": 1, "text": "ab\nc\neo" },
                    { "offset": 3, "length": 4, "text": "uj" },
                    { "offset": 9, "length": 11, "text": "p\nepp" },
                    { "offset": 22, "length": 199, "text": "m" }
                ]
            ]);
        });

        test('gen17', () => {
            runTest("checker-10.txt", 52166, [
                [
                    { "offset": 81, "length": 1, "text": "tmx\nra\ne" },
                    { "offset": 123, "length": 20, "text": "\nkup\no" }
                ]
            ]);
        });

        test('gen18 - split', () => {
            runTest("checker-10.txt", 112, [
                [{
                    offset: 191,
                    length: 15,
                    text: 'fdcikriiawh\nnrjoetvoutbsygijyxgswiwffmmujejrxahlpq\nxarqqujywjchvgenmlryaaljgozuwfllctaeypltvpcoivqrjguqulxgrpzvfr'
                }]
            ]);
        });

        test('gen19 - split issue', () => {
            runTest("checker-400.txt", 320, [
                [{
                    "offset": 9825,
                    "length": 6071,
                    "text": [
                        "vozacoymjbydesaqyotuaqzydf\r\nznqwrvrfgafzpwiirirtpusqzwmyenxtvnxgdprjrxebcmyaaa\r\njgrasesfuoefnres\r\nxksamgrchiqqywazgjqtdebtzsgzbfntubeazkbsnbeaqmwffyacxmllmoqjtah\r\nlbr",
                        "abbhzwexugreosbems\rzyde\riuwmxxrbwacnmxcxydxbgnanniascehgwuroytpasldzjgajwrtuomqsdiybxgmgskiaqerbnxulvycqdzasep\r\nqkappghdvihmjuerkkjiapmhgtwgjoeqwkqdplqkqkxymamhtkncgzvngt",
                        "acumvqpwolol\rumobpcmffrlcmur\nqxfpjmkqyybnggwmtvoijgsomuwkdvqamabfdqsojagwpifqcf\r\niipocqoniweizegxzlydgrwsjxnpoutkglyzczetdiumcamgbvfvehsekfkzjnzxdwbhasavorupxtmgqslpozstr",
                        "\rvsmxqrjkyxyevhaavqbczsvbaetiwkkrwelztjqmauphkwgdgoqxcoyufgihyraykbbiehcqcktuuvcqoczbau\r\nkvdmjkyhqlncpkwyzpeyfwutryizrqiassnkqdvnwkdhvmhvznvybint\r\nddkfkecwibxxsyjeujcuth",
                        "fjnbytatslxhzfjirfrdkkdwoogpdpjiqoltzfibelyqctitpqjvropohitg\r\ngdvqzymibitvthvqtrntqvawjuvqxbryjlevmeosygjmljonkiajnutdifskooovsehcswchjtzvzkiuxaemkk\rtxguxvefgwiaqfuwsipats",
                        "epeelbnfezvbydmogqblnjcxfkjpzfa\nrvhliidfduyzqweofiqzcastetygzfpzqwuzdocgddzfwkxgtxlnyparodgtnohrldazfnihmh\nausutrqxmlsjgxuzchpouiutiqu\r\nejombrncvousem\rykdzcjdnszbzvgdnzb",
                        "jgfvioptusysfzotsukzavssbvwgrxrt\rqgbmlujipjrxzfcpymfnz\r\noavfdlmjugzwtdsmzixxdrawsoqqinuy\rqozasuvvivftfazthnclicesbiajkxpifnvtynzprrlgfthuqoepwjhzeiupbwpfmftvdqjz\r\nmhtyf",
                        "rvkzdnjuijhrcqvtjxuwqhhrphkrzrutwjptxuvrgcvvblvjth\nwrfrljxh\rqegeqdclsoiohbgteoizdipnlpkbbfsathkohvqvorwsckowv\ndb\r\njaeawekuvncecxtuybudruvhisoayfuiccxuobexkajemidddcnbwfa",
                        "yieifzgryljyjknoyepgndd\r\nvcguruljigowbbzcahtvizrvaduwbkdvglqioiulugdwggbetpixputylcigvxiezjdpoausbedvqrkypbysnwwfez\r\nlwstcwconpgnihhinphxjalxblcajaybcxvhstcfxdrniwqajhinx",
                        "cwgzfrhsjqcapgvfzxmqtxaedxnlsxbayzpnj\ryoemcnpnn\nwehppkrxunkdkbegkrjxidffjbchjkepsxdstrpljfeelhqvupekygubvjnlkrgknjnuxffeglsu\r\ngaytqslgsfyheoogznrbxghdlihplvgumwfigxftsrlb",
                        "rjh\r\npdajhjsjslfxzcpqghbdyuflhxlfsqosxe\r\nsmgwfikdqsxemdbmv\roumefegsdjthqmsetbqtlaakwnqxcyldzmjoonkeeiigxxanvihjgjyjqu\nvypxfrvexaafcwhnnprehngjyzfoxcmtrruwsmjo\nrrzs\r\n",
                        "vnmffhvonarwyjwjfmobcdipxztwafcxshxddpftnirwahnverjvmyaavggroalavdz\r\nlpakmtbtjbavupyawoxav\ngekjmvfjnhwuyzondujtsppstybrgwtvvysdcw\rfdbzqahnt\rihuegnuboglcbyyoyeutclgmgpdev",
                        "acjomtrytbllhcfdhcsqjizshlbhzmxjbztfxpatdfwxjw\nkpsxmkqesgammv\ntmjmazdpygnggceksbsejtczhsnsyflsbpgtpbiqcvxclwsdseehdsrufauxqptaeppzlqvtwlqkszgpbsbnbzzowctjxb\r\num\rkxtwhruy",
                        "vmupxpnrqmgtybhhqirextomrmtbzjnyaqsqiyyqmdlxtopjmmgpqqutoibklnclkdfsiwjxgaxmdmmfnoegewsjo\nxwruqowqg\rdgsdhhyrcmxdxnejskmipixtol\r\npsfgqremayhrduvxyka\njfoqnkvmlxqjecrdbmbco",
                        "nscusyu\novartgpuybnmbtqaldpzwvhlpxlhyiqvfiutpmxlgtohhnohtfxpftxaybxg\r\nw\r\nfuucuibbwajtiraahyylpmowyrirhugutwhtnhsaainizduyjsyarciemzybiismngdizcewvygxcfwzkkcagxhux\rhpkgw",
                        "uittxgcwnjuzedogwcgqcwsiayzprpjxuudclgwcqpwzhjsyrzxekthjwqbqdxykjdzld\rbewvyoksaposhwxnmxtr\r\nqviqgypocmufkpyigurdfvhxaddeqtdieqcfzafxevsilulhgkszlswuqkhj\rwzrhotrevmdkmtjxz",
                        "zskkygvqltdczmcexqkoqt\r\njhpjaizyepibgtnwnziygwfoerxrppapxgsdtarnwasa\nbbdlsjygdodyxcsmenmplrxecjpgzvkmjdbwlgfwgrcjmzlylunobadgzfvalgjnjkcgxphtecgonlhnlidbvloiobq\n\r\nbsvxf",
                        "bqbzypduwdaeryjkxennsaxrykmzyzmypqxgwvqpbzityq\rihyjnroeqhjjnztlyesajzlqqpjjgnajchmdbydqxkqivvhxurbj\nizvwycycyuvyriuwkukpgooxjywrwyubaeayagrfffowaptcvrryewxbycjbvyezsotomkcn",
                        "mixswzmp\rfwbfdbglulypkezxacvynprnxl\nhmjvgydgvqnexeptrvgnhxbipdoayzb\r\nvnmxshxfxbwppeoxpwpeheaccekvwcjiaozbcrkmnvfifggdzldjunzhpqxizwdzofufjtp\nkkockahqznyrgrdiuababkbvrvtr",
                        "njpbwcjeflpofufmxkosaqothpoifk\rsyrdnqntzysuqekuwizztekgvvlpfhlqaycrluufggbcqwitqjiighkyieemfvqrbaowdxyxbaqxqtyvoaj\rcmrulvhftzpfhwglzgxwuastilbqytldyzvldwguujsdxnzlbbajrukht",
                        "ecfnrapsittzho\nodaiwpicwmefixnshbrgegkcoqupujpjvqhiahgsiaczqulypkoxojzgmpigexzpdccxmmhbgmeqbyelgivxlbbfprpqbcw\r\ngcklzuz\rcwuqvrqyncvhapnaecdikyjjkvqxgacwygcvohxhzdz\r\nyao",
                        "tjsntvlzysgnxkew\n\rmykixyvyeserksorqdzhguiprltckqgecrdydvrvzwhrzqhtjyxiuqkiiyzllwmzica\nesvfsezdsvolqgnbwmausfioiogjlzpqrcipnhyfsolgu\rgamuiqcxivsjczhtbwtarzhekkaphyehpktxfy",
                        "vcjsiebagawv\rpmsyztco\rdcrqfgzithqsfsdrbnsvpcxrifluzajphwglexqzdpxnqqtmkghepdjuuwoxihnrudnzaeshpxcnqjdhnupqnoioihfweah\r\nxuuaygjoaepcvpcdyjbjrwrtvlmnuasdheejemshm\r\nkbffun",
                        "hguvflukvlcndzwlkxrzvssipnywvxuja\rvtkhl\nbopxgylholvqgccxctxgmelvcbwcgg\rkpqaamnvmelnyuuhdcnrgerc\nichgjgdhctwibaucszkbrtihhwxfrndvizbdtmlhbkl\nbgblneztojnhvh\nlnelijcqhfvko",
                        "bdlxghbeqmxxfsyj\rlnvujjzdwaodeovlloihymuimrqeecroxmtgfwrmfqoeramyacyufxmlqaa\r\noiffokuobuxldcnlkvmmdwkufoimkdkvvzcdpmdkeetvsrjedwhdcfnhhhjjvzwgqz\rehfzrgoexhtqhgxefgyhfvire",
                        "awwtryulpoke\nnjxasqtlshizulqyoolljahkumctdsqgkaydjbzcksxcgxxgdvujymctaneogfauecwyripzfgdbipoewospmooxosrpuqkqb\rnsgbdsqrepxkrlejfxtrurgniuspbqkpemrtbmbvxtudbtrhnsytq\neqgtrb",
                        "rggdzejufuqdbghvgfbnysygsguyppsskxswtcuousvzdikrtpmaabftyokedydxjifvkhpvwafkxgszquiaqmimstojrp\r\njer\rtfyfhjleqtjezpdghqvfimnackonbysrklhhdkoroupodatvkkclyevygqlavytlehrfrcu",
                        "gbsmulvcn\rzrsylljluasimeoiomzlxhlaljdckzpflvoficenvbasmecobecddegovauucbttxbqnnvwxdmwshjdpswda\nkgodvsudjtlurgpwuzjldshp\r\njltdllgckyesoxbmuscotrimrlxyfzpuypmmwvhquhofrvmwn",
                        "fvgihmcwtxwrvumxeo\noudmvbxyuz\niyenoigwwvfoopuhejzjxwqdrjyngskyfhtsggmbbnnfbbfsax\nbefqzgnjqxegkweoxflfzyvthycxbrbpphildufxwnhnveyiohgsuxnocrymmygcjp\ntgvskvwsszqmseqjeqvwwd",
                        "bdfklrvquushkxvpjejntcvsviozhmymfeicayqlslykwkmjn\noxtygqkvohnfrzxtmugmjilrzzmbsptqopuyrazrmuqpwenjxeblbjptjrvg\r\nfdvljsotbdaymtfpxpnfrddssrytzrgsgnqmji\r\njrylalajtylddcjfr",
                        "tpawfkba\rsilxmrtlyyvsjlpbcnltueskbpyujrmw\r\nhgiljygyxcvrrpe\rih\nygxkphrepvzkigoafaebewbcvcmlbubxyiqouhefmstonxfzpgrz\rvaomqlpzczdaxoypzjkgy\naoihxumshvkhmzru\rzistoaboiqpq",
                        "dfiquncwtgwqyobejkegjhoewsvbkfzhi\nksnowrdctealwnqaqqmprthnbgbayrkztylxfumneelglsmdazztpppxjtko\r\ncaouxuwzfjvrialzxnfxkyxabgdufxojnafsu\nvzdgqyzlmeaykvgyepfeobixzmglckhw\nbf",
                        "jixrjyyhqkckdpymyjvrisxkupxhugaoqxgaigbsrskkvu\nzqmxwqlqcvfcclmynulohwracl\rcyjtrwkxcqljokzyqonidzdlfxmqrlbcipfcsqyedfkonnfmuad\r\nhacffjinaylcrpgmmruayaksrkkjwcuzbrtdklkd\r\n",
                        "fqbfptpceuuekzjbdsugzvloufickfxqxjyeighruygjiiznqbzkgboszrez\ndjgluwtwzaxruoqexsdjdnqrt\robnmlwshgkfrckuhusqjcqkyvyymabzqiosrrmyth\rzykujpbxfwnnvmqxyltsnwugihejyn\r\nzlzqzaxz",
                        "rnzqlxatgdtydnmxrlnkupzqpundwtvjnmgdkjxhammfjhpueppcqmfupdbrmunzyirwazzouaafcydnabk\nbeykfrftxwomrkzuxhlqxmcqywyunlwxf\neba\rfycqrfwsihnltnfajknouklqgoeiahuttvzcjzyobxjhcxtbp",
                        "ldlrafbzsbydzdqcfollxctuavhndvmpthvxuarxy\nmasllvytqklexyynerwveohuacvhuplhoedjwhorxqqcctxdaswmlfyywauuddhxfjeah\r\nisnpszuvsxzbkvtfjxxktmwoxgjaaphcrwtanhjosekzkrqocysvffqefg",
                        "lcv\ratfllpmrvfvibekkkgpkaptljqtrnqzafsmvbscxhyawsssnjshssyylnitbedyujsro\r\nvdkohfpdvtibqzhtarhnqqhrcmqoeugkvkkxucpxktykknrhuabmhdfnsuffzubicmzptydgoxbtnuqinpaxtzalhoft\r\nu",
                        "nampjr\nxurfodsrypnyanuloaldoffvkxpdnfahvmuswahgczcbzgofhtwjyznaeohxepvljazdcqasi\r\njxdcficdrnwculdwyhxtryioidmoqmisukslelgjegjwxgwpoesybsovqoietcaaftimhkwpniuaakvyvvnlj\ngq",
                        "wszbecfvfelrxwgcbjzaainsjfptshiuxnqkqzmdpqaitpkqgpdlskuedeqpcowezzo\nqsaczsvhtwpmhdwnqzfbbuigz\r\nhjvdxzlkyddmfumoxtrjubbmgnmzn\rqmbryohtsqikyfshblzcndxoeonxrsuawlzquledla\rx",
                        "xfglkyhuxwoxmqbqsbsfzgxrwqzcvbmiwrnxzaprxxdcijuaueusmcnmbvjwh\n\r\nvlajjtefnbxmrlodxfnesvue\rmd\rzwmuh\rzyryfriklfrxzvhasdvuhlcwseapno\ntdemykvvqevkgerbnaykoiqhcgzlkfyusewnwe",
                        "cqdebm\r\ngaggbpocbeyfdscwzqilaiwhtkrarpfkxfxuvwiepxhcniwqxegpvuhteiwaryqvdppfyvhvvtbk\rsiwlfgsddoreeorsdipxxprsepareikcigzhifwqhfibxseconakdwjtyfrlbw\r\ncrksbrqebfjrivvoaucd",
                        "vbfszvenfodmynr\r\nzaxmgxhyucnyrwafbloysksxzrkmnrcsbizstfewtbiacpvcfjehgfydupwflcgppyufkspyyywdvzetwrnpvksij\r\n\rrxfcjxstxhvafhnumwirrktpntbsjksyehqcrskgjlybetbqjbibgumwlnjt",
                        "hfpcmqnslmgsymnqmkmpmueorntnaixmqieecmqj\ngptpnuhtwlwitolzpqgtggccjxsfrjjorfjqmotobfmbqwmwfmdndrtmtclgombmhewqiaalyn\rkqzzvcrzlniekq\ryfhbtjsewunycliccdfzclqiqachbd\rshdlzxyx",
                        "engzcbctnrcemlsznojbhbbsvrqonnx\rufnqfrrqsdttmmzndeiadhwjsxuodvwjvogajgxmmnoumxzykwaqbointktlhtokwptekofywntcwlerrzqqc\rfridrdcfqmxeujzaymlhapdthczmtrllxo\rpyswzgpqzaejdtoxut",
                        "wgtjykjwi\neklvgvupysffpodbmmjjzpqlgwynhxgdrhacpoanqmbukcieelkhddtxlxfcbhsxykxrahhvtehauwbyjrkoabvbq\nxebjmdyobjkcgbc\nonujwrrainczuravgwfyhhzxfsknnlgmhieqzdmw\r\nhdwsbymjmsa",
                        "mksvdffqkbkiuontjlbgvofdqvzzcftywjafuygz\rjgdeyvuwwtmioigsckrafveidgq\n\nouezqrzhsgsqpvjyqt\rrcwscynrdnmjfqknbkcialegdednclwusxzmqnguvnxxcjnymiqtkvutmhsbcebsodfxhlgmeqyngpcvc",
                        "vxwmtxljruie\ntwkadmbudwcpnouvgdzmxynqwrzyw\rxqsxljwmnifutaigmymldrpwpjqrmgqdqgyivecremullnwpznqzvimwfdnpzqjwinkcetxcnhmtmmmanqz\riglecubnrtaebqvjxgjsk\nedziplkrxqjxjbmswssp\r",
                        "pcmfpxsnlokoibrkxcbrbpyrqblvmgkapnhlhaynbxjyyvcxedpwakommdgtda\nrprnvwwxurptjgtrusukcijsiotwcwwqtffnttgd\nahcbezvynoxfzfnrjxfnenlghuqyq\rwyexkbnwuxbywsgludigxoqymcfnlslvqwikm",
                        "\r\nxmwjwrzskltkjxaghzqycvvxyguqzujhjbwgxkfsjbsqdwbjoyxtzihppjtbwleujabyhoszmfwwveyuylgkmhefehxccbafix\r\ngiruyokaymdkctjowwhpoplxptphoikbithhdilfwvvuqcttqfzvbd\rrtlxxaelrhlv",
                        "ypmzfdecztotxxinzygggbhqxqfewsxvplxnqfpknnnhlfljcmatderpftimytc\rkwqszxcpmuxoinuxlinz\nvfcpbgmwfrqjnvuswlgolgjbbflvfqvjznxfpmcohctrjbrkbisponcqdfycsccrszblpjfnqqdhgrvsotbddwr",
                        "cvhqmnivwz\r\nuzutfqibkwksrluviudhlnfgbsvdpxxbtdgiiadeunoqtebxnqovcvjxjwifhrcemooaevtpbfnclpkchjyodffnakr\r\nqdquzrsijyukehjqrawbpywibeykjuhktvejgburpvrgqksfxfvxfvbk\radtxrvr",
                        "hxxyympbcfjjlvgfcgxvvxmwuzfxnnksrb\r\nwaashovtjyzzkbclgekldvxrynwpnqarcvmxffvtbmjlfwosczhdjrwgvkcflcaitghvenrjzkldfwtokvqqybhh\r\nfdoxceltcqdrogsedgnsobgxxjrokjbjjfmnsi\r\nya",
                        "unukwsykszzogsgceposmteavpslgtlekchsinsbilpinubjfrjecsibznlltizxigmlnequmbjqbltqpjkmsjkhjmai\r\ngofulbhagfhavoymjxzy\r\nrjppnaehnuglllcah\nnhsualdpdfvyrxhmxcyowifktmwwvfuyj\r",
                        "ocwrpojnkrfjdsofvfmhyxbyhoekzkmovrmoywwwm\rfttvzznkbmkxgwfwsjdtffvxqatxkwlvogpxn\negrveymlayazdhjtsrxoeexatlsmyjpeyaaliqsvioxjdpzbig\r\nqlrdskshzocnjkgkcnqkbdttyaf\noqpyegied",
                        "tjcvyzuftpawttuorhcrnzlwshpkh\r\nkeciammhlonteqigjbxunevcgbcohfkyiqdnuffktaeuqywjpvmlkkgajjafbxshmqocccdwq\rlewwtkpmaqxzzxlltddwdwcveawyocgvzliwkqcfoncdpxjfsddjuzvsybzxwiiiob",
                        "hkviceaaqrjs\r\nxxvoxqtriiygmbivtwdpjooozjwwlyjbmrcjkmqagovbjlcbdkmmnbendltahwgwkltlclpsvlmlhjrozlbgrpnmxrikr\r\ntljzwtoxixvpnajtnqltqxqdmpealsrvucrhltxj\rmndbzeqrthnjbnbjcoy",
                        "ebnewkaqxqinzvenxaxcevsuvixxhxjdxatmebjipkimyctjbkjkliznswtlfarzgc\nfsixpqenhuceafyawkcadpsczepbueibqzrxpyvrcmajsjfrssufxlfgarfesnonzxutyqvizqlpummitwtlcgqnnwvwjosvzo\noungez",
                        "lsqflhsxapyuspgtsprbbrlydiwpzbqsjxmmpvyodsizrbkyllgzdftcvceooapfltuktxlztilspboawkbgtzpoiohcl\ntrptyjlkuinxseqlnzirnaphpiowcfumyidkvovupbqpifmzjpe\r\naxaaegselusiphifclefwuel",
                        "icrfbuctkey\ngkcisxxaljryfmjjpkigkfcccusuphxsiaqihidmkvirmpenlyiduwymlxgprp\r\nnrnmeufnlqmpqknsjikeqnemkdkjschwxpflddmajgvoxgvuqhbadjhlabzgaamifwotqufvxbykzxlmbqk\rwcoxoly\rd",
                        "xfitwnwjvxrhaqlkaaavyamtawbeuulvesqummgxchntqij\rutefhnfounuofqxyqfvoigrcvzyhzfdnbslasmvbovruhimeazybhijgnocsgeecpteolk\r\njcodftfrcrhvskbavppwpuueyrjibaejgtf\rwvsfutqzejpllc",
                        "gxgkkbsveiqew\rzcpoxvdpzdqoihwhykunwcvkdaabyrufxkyxbjgkemswbfgeyvfenbchxgfkxekkq\nixzezavmripvswvvsbogcdrewzaplrhsbjgzbnbzfxgyvynliobdnedagstjjpdr\r\nssqvptz\nmykcllomejqvmvw",
                        "rltvzj\r\nxldzlisnmzcypfqutsqefgyaqvtlewqvsjgdptz\r\r\ndxzbvvwabxlvzpydjflrgvmhlgjdheax\nwdosidhwcpimddziaukwvdikbzvyrdplojnymiydniayuwnqdqrbnykkj\rfnmylxt\r\nfdgghmtmevqtcen",
                        "mgwibqhlqmcwbwihkosaugj\rsmsuwoklctvyhlwzaabgnhppufbyftdpjggpbbhvyxgwyuxclnzdpjzydbcpc\nuabndgmiuwsfyokckbmsrztcresjuvrhtimobiwnvjecgqlobrkzarrtbnvvmdntoxqobdsktqecomyyuwdwop",
                        "eutnq\r\ncqvnilxuvnjgwfeczviwtkijkmixsunlmdvgktiawlrlpqjngrxyksyqzhfepcybxt\rggvnynocgwikozgn\r\nhcbxxnhvasqbixhjeadydcuagtlikaazbpmrephkqvqnmdprsahcrpdzkkpuwykekaauefublkvta",
                        "dktanohzchjwf\rkuzbuhtjqabchuxlxddzwfnkjvygwrtjadmrvdbwkdfxgrw\r\nckmbyqlaoawmkzaivxdeqdrzfpljhciyuoafkabxarcmckqlikiwvhufgimtkeqxabmbbnnhirl\nunecoligekanemxbkfqlkxklewazahz",
                        "antbtgovowuxxcxoxootfehmhtwbskannykfqnbwepfaa\ntjqcljubucoriblcjbxflfyiahcsrsxtjkodysagbyigdktdthovfsaodwftprwmeujtmflwnpdtbqnmvdzvt\r\nngdgcvarsoynewohsqbukgfpdsbvpjwxjspczt",
                        "krntceggxlnmofykbdaygzsdeqskehagr\r\nbscmnroypfascbgappnceky\nbrnrpfrkxijltwsdomonxeqsp\newjqaxobmbjakmqxunsycqkfg\r\ngtcnbhjgqckrrulcswtlbxvj\rzpx\nlsbaytxdwpfbq\nbypaehyeha",
                        "toctcfgzrupjtqgfediomekjdobsamixackgcf\ngpmfgvlvqstvfikv\r"
                    ].join('')
                }]]);
        });

        // GENERATE_TESTS=false;
    });

    (function () {

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
                    let edits = generateEdits(EDIT_TYPES, this._content, MIN_PARALLEL_EDITS_CNT, MAX_PARALLEL_EDITS_CNT);
                    if (edits.length === 0) {
                        continue;
                    }
                    // console.log(edits);
                    this._edits.push(edits);
                    this._content = applyOffsetLengthEdits(this._content, edits);

                    const time = PRINT_TIMES ? process.hrtime() : null;
                    this._buff.ReplaceOffsetLen(edits);
                    const diff = PRINT_TIMES ? process.hrtime(time) : null;
                    if (PRINT_TIMES) {
                        console.log(`ReplaceOffsetLen (${this._chunkSize},${this._buff.GetLength()}) took ${diff[0] * 1e9 + diff[1]} nanoseconds, i.e. ${(diff[0] * 1e9 + diff[1]) / 1e6} ms.`);
                    }

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

        const GENERATE_CNT = GENERATE_TESTS ? 20000 : -1;
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
