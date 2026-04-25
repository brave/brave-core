// swift-format-ignore-file

//
//	————————————————————————————————————————————————————————————————————————————————————————————
//	|||||||||                       Swift-Big-Number-Core.swift                       ||||||||||
//	————————————————————————————————————————————————————————————————————————————————————————————
//	Created by Marcel Kröker on 30.09.16.
//	Copyright (c) 2016 Blubyte. All rights reserved.
//
//
//
//	——————————————————————————————————————————— v1.0 ———————————————————————————————————————————
//	- Initial Release.
//
//	——————————————————————————————————————————— v1.1 ———————————————————————————————————————————
//	- Improved String conversion, now about 45x faster, uses base 10^9 instead
//	of base 10.
//	- bytes renamed to limbs.
//	- Uses typealias for limbs and digits.
//
//	——————————————————————————————————————————— v1.2 ———————————————————————————————————————————
//	- Improved String conversion, now about 10x faster, switched from base 10^9
//	to 10^18 (biggest possible decimal base).
//	- Implemented karatsuba multiplication algorithm, about 5x faster than the
//	previous algorithm.
//	- Addition is 1.3x faster.
//	- Addtiton and subtraction omit trailing zeros, algorithms need less
//	operations now.
//	- Implemented exponentiation by squaring.
//	- New storage (BStorage) for often used results.
//	- Uses uint_fast64_t instead of UInt64 for Limbs and Digits.
//
//	——————————————————————————————————————————— v1.3 ———————————————————————————————————————————
//	- Huge Perfomance increase by skipping padding zeros and new multiplication
//	algotithms.
//	- Printing is now about 10x faster, now on par with GMP.
//	- Some operations now use multiple cores.
//
//	——————————————————————————————————————————— v1.4 ———————————————————————————————————————————
//	- Reduced copying by using more pointers.
//	- Multiplication is about 50% faster.
//	- String to BInt conversion is 2x faster.
//	- BInt to String also performs 50% better.
//
//	——————————————————————————————————————————— v1.5 ———————————————————————————————————————————
//	- Updated for full Swift 3 compatibility.
//	- Various optimizations:
//		- Multiplication is about 2x faster.
//		- BInt to String conversion is more than 3x faster.
//		- String to BInt conversion is more than 2x faster.
//
//	——————————————————————————————————————————— v1.6 ———————————————————————————————————————————
//	- Code refactored into modules.
//	- Renamed the project to SMP (Swift Multiple Precision).
//	- Added arbitrary base conversion.
//
//	——————————————————————————————————————————— v2.0 ———————————————————————————————————————————
//	- Updated for full Swift 4 compatibility.
//	- Big refactor, countless optimizations for even better performance.
//	- BInt conforms to SignedNumeric and BinaryInteger, this makes it very easy to write
//	  generic code.
//	- BDouble also conforms to SignedNumeric and has new functionalities.
//
//
//
//	————————————————————————————————————————————————————————————————————————————————————————————
//	||||||||||||||||                         Evolution                          ||||||||||||||||
//	————————————————————————————————————————————————————————————————————————————————————————————
//
//
//
//	Planned features of BInt v3.0:
//	- Implement some basic cryptography functions.
//	- General code cleanup, better documentation.
//	- More extensive tests.
//	- Please contact me if you have any suggestions for new features!
//
//
//
//	————————————————————————————————————————————————————————————————————————————————————————————
//	||||||||||||||||              Basic Project syntax conventions              ||||||||||||||||
//	————————————————————————————————————————————————————————————————————————————————————————————
//
//	Indentation: Tabs
//
//	Align: Spaces
//
//	Style: allman
//	func foo(...)
//	{
//		...
//	}
//
//	Single line if-statement:
//	if condition { code }
//
//	Maximum line length: 96 characters
//
//	————————————————————————————————————————————————————————————————————————————————————————————
//	MARK: - Imports
//	————————————————————————————————————————————————————————————————————————————————————————————
//	||||||||        Imports        |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	————————————————————————————————————————————————————————————————————————————————————————————

import Foundation
#if os(Linux)
import Glibc
#endif

//	MARK: - Typealiases
//	————————————————————————————————————————————————————————————————————————————————————————————
//	||||||||        Typealiases        |||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	————————————————————————————————————————————————————————————————————————————————————————————
//	Limbs are basically single Digits in base 2^64. Each slot in an Limbs array stores one
//	Digit of the number. The least significant digit is stored at index 0, the most significant
//	digit is stored at the last index.

public typealias Limbs  = [UInt64]
public typealias Limb   =  UInt64

//	A digit is a number in base 10^18. This is the biggest possible base that
//	fits into an unsigned 64 bit number while maintaining the propery that the square root of
//	the base is a whole number and a power of ten . Digits are required for printing BInt
//	numbers. Limbs are converted into Digits first, and then printed.

public typealias Digits = [UInt64]
public typealias Digit  =  UInt64

//	Bytes allow to initialize and export BInt for operations like network related ones.

public typealias Bytes  = [UInt8]
public typealias Byte   =  UInt8

//	MARK: - Imports
//	————————————————————————————————————————————————————————————————————————————————————————————
//	||||||||        Operators        |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	————————————————————————————————————————————————————————————————————————————————————————————

precedencegroup ExponentiationPrecedence
{
	associativity: left
	higherThan: MultiplicationPrecedence
	lowerThan: BitwiseShiftPrecedence
}

// Exponentiation operator
infix operator ** : ExponentiationPrecedence

//	MARK: - BInt
//	————————————————————————————————————————————————————————————————————————————————————————————
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||        BInt        ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	————————————————————————————————————————————————————————————————————————————————————————————
///	BInt is an arbitrary precision integer value type. It stores a number in base 2^64 notation
///	as an array. Each element of the array is called a limb, which is of type UInt64, the whole
///	array is called limbs and has the type [UInt64]. A boolean sign variable determines if the
///	number is positive or negative. If sign == true, then the number is smaller than 0,
///	otherwise it is greater or equal to 0. It stores the 64 bit digits in little endian, that
///	is, the least significant digit is stored in the array index 0:
///
///		limbs == [] := undefined, should throw an error
///		limbs == [0], sign == false := 0, defined as positive
///		limbs == [0], sign == true := undefined, should throw an error
///		limbs == [n] := n if sign == false, otherwise -n, given 0 <= n < 2^64
///
///		limbs == [l0, l1, l2, ..., ln] :=
///		(l0 * 2^(0*64)) +
///		(11 * 2^(1*64)) +
///		(12 * 2^(2*64)) +
///		... +
///		(ln * 2^(n*64))
public struct BInt:
	SignedNumeric, // Implies Numeric, Equatable, ExpressibleByIntegerLiteral
	BinaryInteger, // Implies Hashable, CustomStringConvertible, Strideable, Comparable
	ExpressibleByFloatLiteral,
	Codable
{
	//
	//
	//	MARK: - Internal data
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Internal data        |||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	/// Stores the sign of the number represented by the BInt. "true" means that the number is
	/// less than zero, "false" means it's more than or equal to zero.
	internal var sign  = false
	/// Stores the absolute value of the number represented by the BInt. Each element represents
	/// a "digit" of the number in base 2^64 in an acending order, where the first element is
	/// the least significant "digit". This representations is the most efficient one for
	/// computations, however it also means that the number is stored in a non-human-readable
	/// fashion. To make it readable as a decimal number, BInt offers the required functions.
	internal var limbs = Limbs()

	// Required by the protocol "Numeric".
	public typealias Magnitude = BInt

	// Required by the protocol "Numeric". Not useless anymore. - MG
	public var magnitude: BInt
	{
        self.isNegative() ? -self : self
	}

	// Required by the protocol "BinaryInteger".
	public typealias Words = [UInt]

	// Required by the protocol "BinaryInteger".
	public var words: BInt.Words
	{
		return self.limbs.map{ UInt($0) }
	}

	/// Returns the size of the BInt in bits.
	public var size: Int
	{
		return 1 + (self.limbs.count * MemoryLayout<Limb>.size * 8)
	}

	/// Returns a formated human readable string that says how much space (in bytes, kilobytes, megabytes, or gigabytes) the BInt occupies.
	public var sizeDescription: String
	{
		// One bit for the sign, plus the size of the limbs.
        if #available(iOS 13.0, macOS 10.15, watchOS 6.0, tvOS 13.0, *) {
            typealias Storage = Measurement<UnitInformationStorage>
            let bytes = Storage(value: Double(self.size), unit: .bits).converted(to: .bytes)
            if bytes < Storage(value: 1, unit: .kilobytes) {
                return bytes.description
            }
            if bytes < Storage(value: 1, unit: .megabytes) {
                return bytes.converted(to: .kilobytes).description
            }
            if bytes < Storage(value: 1, unit: .gigabytes) {
                return bytes.converted(to: .megabytes).description
            }
            return bytes.converted(to: .gigabytes).description
        } else {
            // Fallback on earlier versions
            let bytes = Double(self.size) / 8.0
            let KB = 1_000.0, MB = KB * KB, GB = KB * KB * KB
            if bytes < KB
            {
                return String(format: "%.1f B", bytes)
            }
            if bytes < MB
            {
                return String(format: "%.1f KB", bytes / KB)
            }
            if bytes < GB
            {
                return String(format: "%.1f MB", bytes / MB)
            }
            return String(format: "%.1f GB", bytes / GB)
        }
	}

    /// Common prefixes for different bases
    static public var stringPrefixes = [
        2: "0b",
        8: "0o",
        16: "0x"
    ]

	//
	//
	//	MARK: - Initializers
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Initializers        ||||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	///	Root initializer for all other initializers. Because no sign is provided, the new
	///	instance is positive by definition.
	internal init(limbs: Limbs)
	{
		precondition(limbs != [], "BInt can't be initialized with limbs == []")
		self.limbs = limbs
	}

	/// Create an instance initialized with a sign and a limbs array.
	internal init(sign: Bool, limbs: Limbs)
	{
		self.init(limbs: limbs)
		self.sign = sign
	}

	/// Create an instance initialized with the value 0.
	init()
	{
		self.init(limbs: [0])
	}

	/// Create an instance initialized to an integer value.
	public init(_ z: Int)
	{
		//	Since abs(Int.min) > Int.max, it is necessary to handle
		//	z == Int.min as a special case.
		if z == Int.min
		{
			self.init(sign: true, limbs: [Limb(Int.max) + 1])
			return
		}
		else
		{
			self.init(sign: z < 0, limbs: [Limb(abs(z))])
		}
	}

	/// Create an instance initialized to an unsigned integer value.
	public init(_ n: UInt)
	{
		self.init(limbs: [Limb(n)])
	}

	/// Create an instance initialized to a string with the value of mathematical numerical
	/// system of the specified radix (base).
	///
    /// The string passed as text may begin with a plus or minus sign character (+ or -), followed by one or more numeric digits (0-9) or letters (a-z or A-Z). Parsing of the string is case insensitive.
    ///
    /// If text is in an invalid format or contains characters that are out of bounds for the given radix, or if the value it denotes in the given radix is not representable, the result is nil.
    ///
	/// Note: I merged all the String inits into one which is much simpler to understand.
	/// Valid input numbers are of the form:
	///  [ "-" ] [ "0x" | "0o" | "0b" ] { radix_digit }
	///  where radix_digit = "0".."9" + "a".."z" + "A".."Z"
    ///
    /// Common prefixes are supported and stripped through ``stringPrefixes``
    ///
    ///  - parameter number: The number to convert
    ///  - parameter radix: The radix, or base, to use for converting text to an Big Integer value. radix must be in the range 2...62. The default is 10.
	public init?(_ number: String, radix: Int = 10)
	{
		var (number, radix, sign, limbs) = (number, radix, false, [Limb(0)])
		
        if radix < 2 {
            return nil
        }

        // First we figure out if the number is negative
		if number.hasPrefix("-")
		{
			number.removeFirst()
			sign = number != "0"  // is zero signed?
		}

        // We strip the number if a radix is set and is prefixed
        if let prefix = Self.stringPrefixes[radix] {
            if number.hasPrefix(prefix) {
                number.removeFirst(prefix.count)
            }
        }
		
		// Reserve enough space for this number Limb.max
		let digitsPerLimb = 64 * log(2.0)/log(Double(radix))
		limbs.reserveCapacity(Int(Double(number.count) / digitsPerLimb))
		
		// Handle most radices the same way unless a radix > 26 is used
		if radix <= 26
		{
			var maxPowerOfRadix = max(1,min(Int(digitsPerLimb), number.count))
			
			// generate a multiplier table at the start
			let multipliersOfRadix:[Digit] = {
				var multipliers = [Digit]()
				var x = Digit(1)
				for _ in 1...maxPowerOfRadix where x < Digit.max/Digit(radix) {
					x *= Digit(radix)
					multipliers.append(x)
				}
				return multipliers
			}()
			
			maxPowerOfRadix = multipliersOfRadix.count // adjust if we guessed wrong
			var chunk = ""; chunk.reserveCapacity(maxPowerOfRadix)
			while !number.isEmpty {
				chunk = ""
				for _ in 1...maxPowerOfRadix where !number.isEmpty {
					chunk.append(number.removeFirst())
				}
				if let num = Limb(chunk, radix: radix)
				{
					limbs = limbs.multiplyingBy([multipliersOfRadix[chunk.count-1]])
					limbs.addProductOf(multiplier: [1], multiplicand: num)
				}
				else
				{
					return nil
				}
			}
		}
		else
		{
			let validDigits = Array("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
			for char in number
			{
				if let digit = validDigits.firstIndex(of: char), digit < radix
				{
					limbs = limbs.multiplyingBy([Limb(radix)])
					limbs.addProductOf(multiplier: [1], multiplicand: Limb(digit))
				}
				else
				{
					return nil
				}
			}
		}
		self.init(sign: sign, limbs: limbs)
	}

	//	Requierd by protocol ExpressibleByFloatLiteral.
	public init(floatLiteral value: Double)
	{
		self.init(sign: value < 0.0, limbs: [Limb(value)])
	}

	//	Required by protocol ExpressibleByIntegerLiteral.
	public init(integerLiteral value: Int)
	{
		self.init(value)
	}

	// Required by protocol Numeric
	public init?<T>(exactly source: T) where T : BinaryInteger
	{
        if T.isSigned {
            guard let int = Int(exactly: source) else {
                return nil
            }
            self.init(int)
        } else {
            guard let int = UInt(exactly: source) else {
                return nil
            }
            self.init(int)
        }
	}

	///	Creates an integer from the given floating-point value, rounding toward zero.
	public init<T>(_ source: T) where T : BinaryFloatingPoint
	{
		self.init(Int(source))
	}

	///	Creates a new instance from the given integer.
	public init<T>(_ source: T) where T : BinaryInteger
	{
        if T.isSigned {
            self.init(Int(source))
        } else {
            self.init(UInt(source))
        }
	}

	///	Creates a new instance with the representable value that’s closest to the given integer.
	public init<T>(clamping source: T) where T : BinaryInteger
	{
		self.init(source)
	}

	///	Creates an integer from the given floating-point value, if it can be represented
	///	exactly.
	public init?<T>(exactly source: T) where T : BinaryFloatingPoint
	{
		self.init(source)
	}

	///	Creates a new instance from the bit pattern of the given instance by sign-extending or
	///	truncating to fit this type.
	public init<T>(truncatingIfNeeded source: T) where T : BinaryInteger
	{
		self.init(source)
	}
	
	/// Creates a new instance from a `[UInt8]` array
	public init(bytes: Bytes)
	{
		var num = BInt()
		
		for byte in bytes
		{
			num = num << 8 | BInt(byte)
		}
		
		self.init(sign: num.sign, limbs: num.limbs)
	}

	//
	//
	//	MARK: - Struct functions
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Struct functions        ||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	// Required by protocol CustomStringConvertible.
	public var description: String
	{
		return (self.sign ? "-" : "").appending(self.limbs.decimalRepresentation)
	}

	/// Returns the BInt's value in the given base (radix) as a string.
	public func asString(radix: Int) -> String
	{
		let chars: [Character] = [
			"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f", "g",
			"h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x",
			"y", "z", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
			"P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
		]

		var (limbs, res) = (self.limbs, "")

		while !limbs.equalTo(0)
		{
			let divmod = limbs.divMod([Limb(radix)])

			if let r = divmod.remainder.first, r < radix
			{
				res.append(chars[Int(r)])
				limbs = divmod.quotient
			}
			else
			{
				fatalError("BInt.asString: Base too big, should be between 2 and 62")
			}
		}

		if res == "" { return "0" }
		return (self.sign ? "-" : "").appending(String(res.reversed()))
	}

	///	Returns BInt's value as an integer. Conversion only works when self has only one limb
	/// that's within the range of the type "Int".
	func asInt() -> Int?
	{
		if self.limbs.count != 1 { return nil }

		let number = self.limbs[0]

		if number <= Limb(Int.max)
		{
			return self.sign ? -Int(number) : Int(number)
		}

		if number == (Limb(Int.max) + 1) && self.sign
		{
			// This is a special case where self == Int.min
			return Int.min
		}

		return nil
	}

	public var rawValue: (sign: Bool, limbs: [UInt64])
	{
		return (self.sign, self.limbs)
	}

	public func hash(into hasher: inout Hasher) {
		hasher.combine("\(self.sign)\(self.limbs)")
	}

	// Required by the protocol "BinaryInteger". A Boolean value indicating whether this type is a
	// signed integer type.
	public static var isSigned: Bool
	{
		return true
	}

	// Required by the protocol "BinaryInteger". The number of bits in the current binary
	// representation of this value.
	public var bitWidth: Int
	{
		return self.limbs.bitWidth
	}


	///	Returns -1 if this value is negative and 1 if it’s positive; otherwise, 0.
	public func signum() -> BInt
	{
		if self.isZero() { return BInt(0) }
		else if self.isPositive() { return BInt(1) }
		else { return BInt(-1) }
	}

	func isPositive() -> Bool { return !self.sign }
	func isNegative() -> Bool { return  self.sign }
	func isZero()     -> Bool { return self.limbs[0] == 0 && self.limbs.count == 1 }
	func isNotZero()  -> Bool { return self.limbs[0] != 0 || self.limbs.count >  1 }
	func isOdd()      -> Bool { return self.limbs[0] & 1 == 1 }
	func isEven()     -> Bool { return self.limbs[0] & 1 == 0 }

	///	The number of trailing zeros in this value’s binary representation.
	public var trailingZeroBitCount: Int
	{
		var i = 0
		while true
		{
			if self.limbs.getBit(at: i) { return i }
			i += 1
		}
	}
	
	/// Bytes of the number
	public func getBytes() -> Bytes
	{
		var bytes = Bytes()
		var copy = self
		
		while copy != 0 {
			bytes.append(Byte(copy & 0xff))
			copy >>= 8
		}
		
		return bytes.reversed()
	}

	//
	//
	//	MARK: - BInt Shifts
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Shifts        |||||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public static func <<<T: BinaryInteger>(lhs: BInt, rhs: T) -> BInt
	{
		if rhs < 0 { return lhs >> rhs }

		let limbs = lhs.limbs.shiftingUp(Int(rhs))
		let sign = lhs.isNegative() && !limbs.equalTo(0)

		return BInt(sign: sign, limbs: limbs)
	}

	public static func <<=<T: BinaryInteger>(lhs: inout BInt, rhs: T)
	{
		lhs.limbs.shiftUp(Int(rhs))
	}

	public static func >><T: BinaryInteger>(lhs: BInt, rhs: T) -> BInt
	{
		if rhs < 0 { return lhs << rhs }
		return BInt(sign: lhs.sign, limbs: lhs.limbs.shiftingDown(Int(rhs)))
	}

	public static func >>=<T: BinaryInteger>(lhs: inout BInt, rhs: T)
	{
		lhs.limbs.shiftDown(Int(rhs))
	}

	//
	//
	//	MARK: - BInt Bitwise AND
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt BInt Bitwise AND        |||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	///	Returns the result of performing a bitwise AND operation on the two given values.
	public static func &(lhs: BInt, rhs: BInt) -> BInt
	{
		var res: Limbs = [0]

		for i in 0..<(64 * Swift.max(lhs.limbs.count, rhs.limbs.count))
		{
			let newBit = lhs.limbs.getBit(at: i) && rhs.limbs.getBit(at: i)
			res.setBit(at: i, to: newBit)
		}

		return BInt(sign: lhs.sign && rhs.sign, limbs: res)
	}

	//	static func &(lhs: Int, rhs: BInt) -> BInt
	//	static func &(lhs: BInt, rhs: Int) -> BInt

	///	Stores the result of performing a bitwise AND operation on the two given values in the
	///	left-hand-side variable.
	public static func &=(lhs: inout BInt, rhs: BInt)
	{
		let res = lhs & rhs
		lhs = res
	}

	//	static func &=(inout lhs: Int, rhs: BInt)
	//	static func &=(inout lhs: BInt, rhs: Int)

	//
	//
	//	MARK: - BInt Bitwise OR
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Bitwise OR        |||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public static func |(lhs: BInt, rhs: BInt) -> BInt
	{
		var res: Limbs = [0]

		for i in 0..<(64 * Swift.max(lhs.limbs.count, rhs.limbs.count))
		{
			let newBit = lhs.limbs.getBit(at: i) || rhs.limbs.getBit(at: i)
			res.setBit(at: i, to: newBit)
		}

		return BInt(sign: lhs.sign || rhs.sign, limbs: res)
	}

	//	static func |(lhs: Int, rhs: BInt) -> BInt
	//	static func |(lhs: BInt, rhs: Int) -> BInt
	//
	public static func |=(lhs: inout BInt, rhs: BInt)
	{
		let res = lhs | rhs
		lhs = res
	}
	//	static func |=(inout lhs: Int, rhs: BInt)
	//	static func |=(inout lhs: BInt, rhs: Int)

	//
	//
	//	MARK: - BInt Bitwise OR
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Bitwise XOR        ||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public static func ^(lhs: BInt, rhs: BInt) -> BInt
	{
		var res: Limbs = [0]

		for i in 0..<(64 * Swift.max(lhs.limbs.count, rhs.limbs.count))
		{
			let newBit = lhs.limbs.getBit(at: i) != rhs.limbs.getBit(at: i)
			res.setBit(at: i, to: newBit)
		}

		return BInt(sign: lhs.sign != rhs.sign, limbs: res)
	}

	public static func ^=(lhs: inout BInt, rhs: BInt)
	{
		let res = lhs ^ rhs
		lhs = res
	}

	//
	//
	//	MARK: - BInt Bitwise NOT
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Bitwise NOT        ||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public prefix static func ~(x: BInt) -> BInt
	{
		var res = x.limbs
		for i in 0..<(res.exactBitWidth)
		{
			res.setBit(at: i, to: !res.getBit(at: i))
		}

		while res.last! == 0 && res.count > 1 { res.removeLast() }

		return BInt(sign: !x.sign, limbs: res)
	}

	//
	//
	//	MARK: - BInt Addition
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Addition        |||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public prefix static func +(x: BInt) -> BInt
	{
		return x
	}

	// Required by protocol Numeric
	public static func +=(lhs: inout BInt, rhs: BInt)
	{
		if lhs.sign == rhs.sign
		{
			lhs.limbs.addLimbs(rhs.limbs)
			return
		}

		let rhsIsMin = rhs.limbs.lessThan(lhs.limbs)
		lhs.limbs.difference(rhs.limbs)
		lhs.sign = (rhs.sign && !rhsIsMin) || (lhs.sign && rhsIsMin) // DNF minimization

		if lhs.isZero() { lhs.sign = false }
	}

	// Required by protocol Numeric
	public static func +(lhs: BInt, rhs: BInt) -> BInt
	{
		var lhs = lhs
		lhs += rhs
		return lhs
	}

	public static func +(lhs:  Int, rhs: BInt) -> BInt { return BInt(lhs) + rhs }
	public static func +(lhs: BInt, rhs:  Int) -> BInt { return lhs + BInt(rhs) }

	public static func +=(lhs: inout  Int, rhs: BInt) { lhs = (BInt(lhs) + rhs).asInt()! }
	public static func +=(lhs: inout BInt, rhs:  Int) { lhs +=  BInt(rhs)                 }

	//
	//
	//	MARK: - BInt Negation
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Negation        |||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	// Required by protocol SignedNumeric
	public mutating func negate()
	{
		if self.isNotZero() { self.sign = !self.sign }
	}

	// Required by protocol SignedNumeric
	public static prefix func -(n: BInt) -> BInt
	{
		var n = n
		n.negate()
		return n
	}

	//
	//
	//	MARK: - BInt Subtraction
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Subtraction        ||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	// Required by protocol Numeric
	public static func -(lhs: BInt, rhs: BInt) -> BInt
	{
		return lhs + -rhs
	}

	public static func -(lhs:  Int, rhs: BInt) -> BInt { return BInt(lhs) - rhs }
	public static func -(lhs: BInt, rhs:  Int) -> BInt { return lhs - BInt(rhs) }

	// Required by protocol Numeric
	public static func -=(lhs: inout BInt, rhs: BInt) { lhs += -rhs                        }
	public static func -=(lhs: inout  Int, rhs: BInt)  { lhs  = (BInt(lhs) - rhs).asInt()! }
	public static func -=(lhs: inout BInt, rhs:  Int)  { lhs -= BInt(rhs)                  }

	//
	//
	//	MARK: - BInt Multiplication
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Multiplication        |||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	// Required by protocol Numeric
	public static func *(lhs: BInt, rhs: BInt) -> BInt
	{
		let sign = !(lhs.sign == rhs.sign || lhs.isZero() || rhs.isZero())
		return BInt(sign: sign, limbs: lhs.limbs.multiplyingBy(rhs.limbs))
	}

	public static func *(lhs: Int, rhs: BInt) -> BInt { return BInt(lhs) * rhs }
	public static func *(lhs: BInt, rhs: Int) -> BInt { return lhs * BInt(rhs) }

	// Required by protocol SignedNumeric
	public static func *=(lhs: inout BInt, rhs: BInt) { lhs = lhs * rhs                  }
	public static func *=(lhs: inout  Int, rhs: BInt) { lhs = (BInt(lhs) * rhs).asInt()! }
	public static func *=(lhs: inout BInt, rhs:  Int) { lhs = lhs * BInt(rhs)            }

	//
	//
	//	MARK: - BInt Exponentiation
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Exponentiation        |||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public static func **(lhs: BInt, rhs: Int) -> BInt
	{
		return lhs ** BInt(rhs)
	}
	
	public static func **(lhs: BInt, rhs: BInt) -> BInt
	{
		precondition(rhs >= 0, "BInts can't be exponentiated with exponents < 0")
		return BInt(sign: lhs.sign && (rhs % 2 != 0), limbs: lhs.limbs.exponentiating(rhs))
	}

	public func factorial() -> BInt
	{
		precondition(!self.sign, "Can't calculate the factorial of an negative number")

		if(self.isZero()) {
			return BInt(1)
		}
		
		return BInt(limbs: Limbs.recursiveMul(0, Limb(self.asInt()!)))
	}

	//
	//
	//	MARK: - BInt Division
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Division        |||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	///	Returns the quotient and remainder of this value divided by the given value.
	public func quotientAndRemainder(dividingBy rhs: BInt) -> (quotient: BInt, remainder: BInt)
	{
		let limbRes = self.limbs.divMod(rhs.limbs)
		return (BInt(limbs: limbRes.quotient), BInt(limbs: limbRes.remainder))
	}

	public static func /(lhs: BInt, rhs:BInt) -> BInt
	{
		let limbs = lhs.limbs.divMod(rhs.limbs).quotient
		let sign = (lhs.sign != rhs.sign) && !limbs.equalTo(0)

		return BInt(sign: sign, limbs: limbs)
	}

	static func /(lhs:  Int, rhs: BInt) -> BInt { return BInt(lhs) / rhs }
	static func /(lhs: BInt, rhs:  Int) -> BInt { return lhs / BInt(rhs) }

	public static func /=(lhs: inout BInt, rhs: BInt) { lhs = lhs / rhs       }
	static func /=(lhs: inout BInt, rhs:  Int) { lhs = lhs / BInt(rhs) }

	//
	//
	//	MARK: - BInt Modulus
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Modulus        ||||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public static func %(lhs: BInt, rhs: BInt) -> BInt
	{
		let limbs = lhs.limbs.divMod(rhs.limbs).remainder
		let sign = lhs.sign && !limbs.equalTo(0)

		return BInt(sign: sign, limbs: limbs)
	}

	static func %(lhs:  Int, rhs: BInt) -> BInt { return BInt(lhs) % rhs  }
	static func %(lhs: BInt, rhs:  Int) -> BInt { return lhs  % BInt(rhs) }

	public static func %=(lhs: inout BInt, rhs: BInt)  { lhs = lhs % rhs       }
	static func %=(lhs: inout BInt, rhs:  Int)  { lhs = lhs % BInt(rhs) }

	//
	//
	//	MARK: - BInt Comparing
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BInt Comparing        ||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	// Required by protocol Equatable
	public static func ==(lhs: BInt, rhs: BInt) -> Bool
	{
		if lhs.sign != rhs.sign { return false }
		return lhs.limbs == rhs.limbs
	}

	static func ==<T: BinaryInteger>(lhs: BInt, rhs: T) -> Bool
	{
		if lhs.limbs.count != 1 { return false }
        if lhs.sign && rhs > 0 { return false }
        return lhs.limbs[0] == rhs.magnitude
	}

	static func ==<T: BinaryInteger>(lhs:  T, rhs: BInt) -> Bool { return rhs == lhs }

	static func !=(lhs: BInt, rhs: BInt) -> Bool
	{
		if lhs.sign != rhs.sign { return true }
		return lhs.limbs != rhs.limbs
	}

	static func !=<T: BinaryInteger>(lhs: BInt, rhs: T) -> Bool
	{
		if lhs.limbs.count != 1 { return true }
        if lhs.sign && rhs > 0 { return true }
        return lhs.limbs[0] != rhs.magnitude
	}

	static func !=<T: BinaryInteger>(lhs: T, rhs: BInt) -> Bool { return rhs != lhs }

	// Required by protocol Comparable
	public static func <(lhs: BInt, rhs: BInt) -> Bool
	{
		if lhs.sign != rhs.sign { return lhs.sign }

		if lhs.sign { return rhs.limbs.lessThan(lhs.limbs) }
		return lhs.limbs.lessThan(rhs.limbs)
	}

	static func <<T: BinaryInteger>(lhs: BInt, rhs: T) -> Bool
	{
		if lhs.sign != (rhs < 0) { return lhs.sign }

		if lhs.sign
		{
			if lhs.limbs.count != 1 { return true }
			return rhs < lhs.limbs[0]
		}
		else
		{
			if lhs.limbs.count != 1 { return false }
			return lhs.limbs[0] < rhs
		}

	}

	static func <(lhs:  Int, rhs: BInt) -> Bool { return BInt(lhs) < rhs }
	static func <(lhs: BInt, rhs:  Int) -> Bool { return lhs < BInt(rhs) }

	// Required by protocol Comparable
	public static func >(lhs: BInt, rhs: BInt) -> Bool { return rhs < lhs       }
	static func >(lhs:  Int, rhs: BInt) -> Bool { return BInt(lhs) > rhs }
	static func >(lhs: BInt, rhs:  Int) -> Bool { return lhs > BInt(rhs) }

	// Required by protocol Comparable
	public static func <=(lhs: BInt, rhs: BInt) -> Bool { return !(rhs < lhs)       }
	static func <=(lhs:  Int, rhs: BInt) -> Bool { return !(rhs < BInt(lhs)) }
	static func <=(lhs: BInt, rhs:  Int) -> Bool { return !(BInt(rhs) < lhs) }

	// Required by protocol Comparable
	public static func >=(lhs: BInt, rhs: BInt) -> Bool { return !(lhs < rhs)       }
	static func >=(lhs:  Int, rhs: BInt) -> Bool { return !(BInt(lhs) < rhs) }
	static func >=(lhs: BInt, rhs:  Int) -> Bool { return !(lhs < BInt(rhs)) }
}

fileprivate let DigitBase:     Digit = 1_000_000_000_000_000_000
fileprivate let DigitHalfBase: Digit =             1_000_000_000
fileprivate let DigitZeros           =                        18

fileprivate extension Array where Element == Limb
{
	var decimalRepresentation: String
	{
		// First, convert limbs to digits
		var digits: Digits = [0]
		var power: Digits = [1]

		for limb in self
		{
			let digit = (limb >= DigitBase)
				? [limb % DigitBase, limb / DigitBase]
				: [limb]

			digits.addProductOfDigits(digit, power)

			var nextPower: Digits = [0]
			nextPower.addProductOfDigits(power, [446_744_073_709_551_616, 18])
			power = nextPower
		}

		// Then, convert digits to string
		var res = String(digits.last!)

		if digits.count == 1 { return res }

		for i in (0..<(digits.count - 1)).reversed()
		{
			let str = String(digits[i])

			let leadingZeros = String(repeating: "0", count: DigitZeros - str.count)

			res.append(leadingZeros.appending(str))
		}

		return res
	}
}
fileprivate extension Digit
{
	mutating func addReportingOverflowDigit(_ addend: Digit) -> Bool
	{
		self = self &+ addend
		if self >= DigitBase { self -= DigitBase; return true }
		return false
	}

	func multipliedFullWidthDigit(by multiplicand: Digit) -> (Digit, Digit)
	{
		let (lLo, lHi) = (self % DigitHalfBase, self / DigitHalfBase)
		let (rLo, rHi) = (multiplicand % DigitHalfBase, multiplicand / DigitHalfBase)

		let K = (lHi * rLo) + (rHi * lLo)

		var resLo = (lLo * rLo) + ((K % DigitHalfBase) * DigitHalfBase)
		var resHi = (lHi * rHi) + (K / DigitHalfBase)

		if resLo >= DigitBase
		{
			resLo -= DigitBase
			resHi += 1
		}

		return (resLo, resHi)
	}
}
fileprivate extension Array where Element == Digit
{
	mutating func addOneDigit(
		_ addend: Limb,
		padding paddingZeros: Int
		){
		let sc = self.count

		if paddingZeros >  sc { self += Digits(repeating: 0, count: paddingZeros &- sc) }
		if paddingZeros >= sc { self.append(addend); return }

		// Now, i < sc
		var i = paddingZeros

		let ovfl = self[i].addReportingOverflowDigit(addend)

		while ovfl
		{
			i += 1
			if i == sc { self.append(1); return }
			self[i] += 1
			if self[i] != DigitBase { return }
			self[i] = 0
		}
	}

	mutating func addTwoDigit(
		_ addendLow: Limb,
		_ addendHigh: Limb,
		padding paddingZeros: Int)
	{
		let sc = self.count

		if paddingZeros >  sc { self += Digits(repeating: 0, count: paddingZeros &- sc) }
		if paddingZeros >= sc { self += [addendLow, addendHigh]; return }

		// Now, i < sc
		var i = paddingZeros
		var newDigit: Digit

		let ovfl1 = self[i].addReportingOverflowDigit(addendLow)
		i += 1

		if i == sc
		{
			newDigit = (addendHigh &+ (ovfl1 ? 1 : 0)) % DigitBase
			self.append(newDigit)
			if newDigit == 0 { self.append(1) }
			return
		}

		// Still, i < sc
		var ovfl2 = self[i].addReportingOverflowDigit(addendHigh)
		if ovfl1
		{
			self[i] += 1
			if self[i] == DigitBase { self[i] = 0; ovfl2 = true }
		}

		while ovfl2
		{
			i += 1
			if i == sc { self.append(1); return }
			self[i] += 1
			if self[i] != DigitBase { return }
			self[i] = 0
		}
	}

	mutating func addProductOfDigits(_ multiplier: Digits, _ multiplicand: Digits)
	{
		let (mpc, mcc) = (multiplier.count, multiplicand.count)
		self.reserveCapacity(mpc &+ mcc)

		var l, r, resLo, resHi: Digit

		for i in 0..<mpc
		{
			l = multiplier[i]
			if l == 0 { continue }

			for j in 0..<mcc
			{
				r = multiplicand[j]
				if r == 0 { continue }

				(resLo, resHi) = l.multipliedFullWidthDigit(by: r)

				if resHi == 0
				{
					self.addOneDigit(resLo, padding: i + j)
				}
				else
				{
					self.addTwoDigit(resLo, resHi, padding: i + j)
				}
			}
		}
	}
}
//
//
//	MARK: - Limbs extension
//	————————————————————————————————————————————————————————————————————————————————————————————
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||        Limbs extension        |||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	————————————————————————————————————————————————————————————————————————————————————————————
//
//
//
// Extension to Limbs type
fileprivate extension Array where Element == Limb
{
	//
	//
	//	MARK: - Limbs bitlevel
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Limbs bitlevel        ||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	
	/// The number of bits in the binary representation of this value, including leading zeros.
	var bitWidth: Int { self.count * Limb.bitWidth }
	
	/// Returns the number of bits that contribute to the represented number, ignoring all
	/// leading zeros.
	var exactBitWidth: Int
	{
		let last = self.last!
		let lastBits = last.bitWidth - last.leadingZeroBitCount
		return ((self.count - 1) * Limb.bitWidth) + lastBits
	}

	///	Get bit i of limbs.
	func getBit(at i: Int) -> Bool
	{
		let limbIndex = Int(Limb(i) >> 6)

		if limbIndex >= self.count { return false }

		let bitIndex = Limb(i) & 0b111_111

		return (self[limbIndex] & (1 << bitIndex)) != 0
	}

	/// Set bit i of limbs to b. b must be 0 for false, and everything else for true.
	mutating func setBit(
		at i: Int,
		to bit: Bool
		){
		let limbIndex = Int(Limb(i) >> 6)

		if limbIndex >= self.count && !bit { return }

		let bitIndex = Limb(i) & 0b111_111

		while limbIndex >= self.count { self.append(0) }

		if bit
		{
			self[limbIndex] |= (1 << bitIndex)
		}
		else
		{
			self[limbIndex] &= ~(1 << bitIndex)
		}
	}

	//
	//
	//	MARK: - Limbs Shifting
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Limbs Shifting        ||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	mutating func shiftUp(_ shift: Int)
	{
		// No shifting is required in this case
		if shift == 0 || self.equalTo(0) { return }

		let limbShifts =  shift >> 6
		let bitShifts = Limb(shift) & 0x3f

		if bitShifts != 0
		{
			var previousCarry = Limb(0)
			var carry = Limb(0)
			var ele = Limb(0) // use variable to minimize array accesses

			for i in 0..<self.count
			{
				ele = self[i]

				carry = ele >> (64 - bitShifts)

				ele <<= bitShifts
				ele |= previousCarry // carry from last step
				previousCarry = carry

				self[i] = ele
			}

			if previousCarry != 0 { self.append(previousCarry) }
		}

		if limbShifts != 0
		{
			self.insert(contentsOf: Limbs(repeating: 0, count: limbShifts), at: 0)
		}
	}

	func shiftingUp(_ shift: Int) -> Limbs
	{
		var res = self
		res.shiftUp(shift)
		return res
	}

	mutating func shiftDown(_ shift: Int)
	{
		if shift == 0 || self.equalTo(0) { return }

		let limbShifts =  shift >> 6
		let bitShifts = Limb(shift) & 0x3f

		if limbShifts >= self.count
		{
			self = [0]
			return
		}

		self.removeSubrange(0..<limbShifts)

		if bitShifts != 0
		{
			var previousCarry = Limb(0)
			var carry = Limb(0)
			var ele = Limb(0) // use variable to minimize array accesses

			var i = self.count - 1 // use while for high performance
			while i >= 0
			{
				ele = self[i]

				carry = ele << (64 - bitShifts)

				ele >>= bitShifts
				ele |= previousCarry
				previousCarry = carry

				self[i] = ele

				i -= 1
			}
		}

		if self.last! == 0 && self.count != 1 { self.removeLast() }
	}

	func shiftingDown(_ shift: Int) -> Limbs
	{
		var res = self
		res.shiftDown(shift)
		return res
	}

	//
	//
	//	MARK: - Limbs Addition
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Limbs Addition        ||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	mutating func addLimbs(_ addend: Limbs)
	{
		let (sc, ac) = (self.count, addend.count)

		var (newLimb, ovfl) = (Limb(0), false)

		let minCount = Swift.min(sc, ac)

		var i = 0
		while i < minCount
		{
			if ovfl
			{
				(newLimb, ovfl) =  self[i].addingReportingOverflow(addend[i])
				newLimb = newLimb &+ 1

				ovfl = ovfl || newLimb == 0
			}
			else
			{
				(newLimb, ovfl) = self[i].addingReportingOverflow(addend[i])
			}

			self[i] = newLimb
			i += 1
		}

		while ovfl
		{
			if i < sc
			{
				if i < ac
				{
					(newLimb, ovfl) = self[i].addingReportingOverflow(addend[i])
					newLimb = newLimb &+ 1
					ovfl = ovfl || newLimb == 0
				}
				else
				{
					(newLimb, ovfl) = self[i].addingReportingOverflow(1)
				}

				self[i] = newLimb
			}
			else
			{
				if i < ac
				{
					(newLimb, ovfl) = addend[i].addingReportingOverflow(1)
					self.append(newLimb)
				}
				else
				{
					self.append(1)
					return
				}
			}

			i += 1
		}

		if self.count < ac
		{
			self.append(contentsOf: addend.suffix(from: i))
		}
	}

	/// Adding Limbs and returning result
	func adding(_ addend: Limbs) -> Limbs
	{
		var res = self
		res.addLimbs(addend)
		return res
	}

	// CURRENTLY NOT USED:
	///	Add the addend to Limbs, while using a padding at the lower end.
	///	Every zero is a Limb, that means one padding zero equals 64 padding bits
	mutating func addLimbs(
		_ addend: Limbs,
		padding paddingZeros: Int
		){
		let sc = self.count

		if paddingZeros >  sc { self += Digits(repeating: 0, count: paddingZeros &- sc) }
		if paddingZeros >= sc { self += addend; return }

		// Now, i < sc
		let ac = addend.count &+ paddingZeros

		var (newLimb, ovfl) = (Limb(0), false)

		let minCount = Swift.min(sc, ac)

		var i = paddingZeros
		while i < minCount
		{
			if ovfl
			{
				(newLimb, ovfl) = self[i].addingReportingOverflow(addend[i &- paddingZeros])
				newLimb = newLimb &+ 1
				self[i] = newLimb
				ovfl = ovfl || newLimb == 0
			}
			else
			{
				(self[i], ovfl) = self[i].addingReportingOverflow(addend[i &- paddingZeros])
			}

			i += 1
		}

		while ovfl
		{
			if i < sc
			{
				let adding = i < ac ? addend[i &- paddingZeros] &+ 1 : 1
				(self[i], ovfl) = self[i].addingReportingOverflow(adding)
				ovfl = ovfl || adding == 0
			}
			else
			{
				if i < ac
				{
					(newLimb, ovfl) = addend[i &- paddingZeros].addingReportingOverflow(1)
					self.append(newLimb)
				}
				else
				{
					self.append(1)
					return
				}
			}

			i += 1
		}

		if self.count < ac
		{
			self.append(contentsOf: addend.suffix(from: i &- paddingZeros))
		}
	}

	mutating func addOneLimb(
		_ addend: Limb,
		padding paddingZeros: Int
		){
		let sc = self.count

		if paddingZeros >  sc { self += Digits(repeating: 0, count: paddingZeros &- sc) }
		if paddingZeros >= sc { self.append(addend); return }

		// Now, i < lhc
		var i = paddingZeros

		var ovfl: Bool
		(self[i], ovfl) = self[i].addingReportingOverflow(addend)

		while ovfl
		{
			i += 1
			if i == sc { self.append(1); return }
			(self[i], ovfl) = self[i].addingReportingOverflow(1)
		}
	}

	/// Basically self.addOneLimb([addendLow, addendHigh], padding: paddingZeros), but faster
	mutating func addTwoLimb(
		_ addendLow: Limb,
		_ addendHigh: Limb,
		padding paddingZeros: Int)
	{
		let sc = self.count

		if paddingZeros >  sc { self += Digits(repeating: 0, count: paddingZeros &- sc) }
		if paddingZeros >= sc { self += [addendLow, addendHigh]; return }

		// Now, i < sc
		var i = paddingZeros
		var newLimb: Limb

		var ovfl1: Bool
		(self[i], ovfl1) =  self[i].addingReportingOverflow(addendLow)
		i += 1

		if i == sc
		{
			newLimb = addendHigh &+ (ovfl1 ? 1 : 0)
			self.append(newLimb)
			if newLimb == 0 { self.append(1) }
			return
		}

		// Still, i < sc
		var ovfl2: Bool
		(self[i], ovfl2) = self[i].addingReportingOverflow(addendHigh)

		if ovfl1
		{
			self[i] = self[i] &+ 1
			if self[i] == 0 { ovfl2 = true }
		}

		while ovfl2
		{
			i += 1
			if i == sc { self.append(1); return }
			(self[i], ovfl2) = self[i].addingReportingOverflow(1)
		}
	}

	//
	//
	//	MARK: - Limbs Subtraction
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Limbs Subtraction        |||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	/// Calculates difference between Limbs in left limb
	mutating func difference(_ subtrahend: Limbs)
	{
		var subtrahend = subtrahend
		// swap to get difference
		if self.lessThan(subtrahend) { swap(&self, &subtrahend) }

		let rhc = subtrahend.count
		var ovfl = false

		var i = 0

		// skip first zeros
		while i < rhc && subtrahend[i] == 0 { i += 1 }

		while i < rhc
		{
			if ovfl
			{
				(self[i], ovfl) = self[i].subtractingReportingOverflow(subtrahend[i])
				self[i] = self[i] &- 1
				ovfl = ovfl || self[i] == Limb.max
			}
			else
			{
				(self[i], ovfl) = self[i].subtractingReportingOverflow(subtrahend[i])
			}

			i += 1
		}

		while ovfl
		{
			if i >= self.count
			{
				self.append(Limb.max)
				break
			}

			(self[i], ovfl) = self[i].subtractingReportingOverflow(1)

			i += 1
		}

		if self.count > 1 && self.last! == 0 // cut excess zeros if required
		{
			var j = self.count - 2
			while j >= 1 && self[j] == 0 { j -= 1 }

			self.removeSubrange((j + 1)..<self.count)
		}
	}

	func differencing(_ subtrahend: Limbs) -> Limbs
	{
		var res = self
		res.difference(subtrahend)
		return res
	}

	//
	//
	//	MARK: - Limbs Multiplication
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Limbs Multiplication        |||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	mutating func addProductOf(
		multiplier: Limbs,
		multiplicand: Limbs
	){
		let (mpc, mcc) = (multiplier.count, multiplicand.count)

		self.reserveCapacity(mpc + mcc)

		// Minimize array subscript calls
		var l, r, mulHi, mulLo: Limb

		for i in 0..<mpc
		{
			l = multiplier[i]
			if l == 0 { continue }

			for j in 0..<mcc
			{
				r = multiplicand[j]
				if r == 0 { continue }

				(mulHi, mulLo) = l.multipliedFullWidth(by: r)

				if mulHi != 0 { self.addTwoLimb(mulLo, mulHi, padding: i + j) }
				else          { self.addOneLimb(mulLo,        padding: i + j) }
			}
		}
	}

	// Perform res += (lhs * r)
	mutating func addProductOf(
		multiplier: Limbs,
		multiplicand: Limb
		){
		if multiplicand < 2
		{
			if multiplicand == 1 { self.addLimbs(multiplier) }
			// If r == 0 then do nothing with res
			return
		}

		// Minimize array subscript calls
		var l, mulHi, mulLo: Limb

		for i in 0..<multiplier.count
		{
			l = multiplier[i]
			if l == 0 { continue }

			(mulHi, mulLo) = l.multipliedFullWidth(by: multiplicand)

			if mulHi != 0 { self.addTwoLimb(mulLo, mulHi, padding: i) }
			else          { self.addOneLimb(mulLo,        padding: i) }
		}
	}

	func multiplyingBy(_ multiplicand: Limbs) -> Limbs
	{
		var res: Limbs = [0]
		res.addProductOf(multiplier: self, multiplicand: multiplicand)
		return res
	}

	func squared() -> Limbs
	{
		var res: Limbs = [0]
		res.reserveCapacity(2 * self.count)

		// Minimize array subscript calls
		var l, r, mulHi, mulLo: Limb

		for i in 0..<self.count
		{
			l = self[i]
			if l == 0 { continue }

			for j in 0...i
			{
				r = self[j]
				if r == 0 { continue }

				(mulHi, mulLo) = l.multipliedFullWidth(by: r)

				if mulHi != 0
				{
					if i != j { res.addTwoLimb(mulLo, mulHi, padding: i + j) }
					res.addTwoLimb(mulLo, mulHi, padding: i + j)
				}
				else
				{
					if i != j { res.addOneLimb(mulLo, padding: i + j) }
					res.addOneLimb(mulLo, padding: i + j)
				}
			}
		}

		return res
	}

	//
	//
	//	MARK: - Limbs Exponentiation
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Limbs Exponentiation        ||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	// Exponentiation by squaring
	func exponentiating(_ exponent: BInt) -> Limbs
	{
		if exponent == 0 { return [1] }
		if exponent == 1 { return self }

		var base = self
		var exponent = exponent
		var y: Limbs = [1]

		while exponent > 1
		{
			if exponent & 1 != 0 { y = y.multiplyingBy(base) }
			base = base.squared()
			exponent >>= 1
		}

		return base.multiplyingBy(y)
	}
	
	// Alias for Int exponentiation
	func exponentiating(_ exponent: Int) -> Limbs
	{
		return exponentiating(BInt(exponent))
	}

	/// Calculate (n + 1) * (n + 2) * ... * (k - 1) * k
	static func recursiveMul(_ n: Limb, _ k: Limb) -> Limbs
	{
		if n >= k - 1 { return [k] }

		let m = (n + k) >> 1

		return recursiveMul(n, m).multiplyingBy(recursiveMul(m, k))
	}

	func factorial(_ base: Int) -> BInt
	{
		return BInt(limbs: Limbs.recursiveMul(0, Limb(base)))
	}

	//
	//
	//	MARK: - Limbs Division and Modulo
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Limbs Division and Modulo        |||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	/// An O(n) division algorithm that returns quotient and remainder.
	func divMod(_ divisor: Limbs) -> (quotient: Limbs, remainder: Limbs)
	{
		precondition(!divisor.equalTo(0), "Division or Modulo by zero not allowed")

		if self.equalTo(0) { return ([0], [0]) }

		var (quotient, remainder): (Limbs, Limbs) = ([0], [0])
		var (previousCarry, carry, ele): (Limb, Limb, Limb) = (0, 0, 0)

		// bits of lhs minus one bit
		var i = (64 * (self.count - 1)) + Int(log2(Double(self.last!)))

		while i >= 0
		{
			// shift remainder by 1 to the left
			for r in 0..<remainder.count
			{
				ele = remainder[r]
				carry = ele >> 63
				ele <<= 1
				ele |= previousCarry // carry from last step
				previousCarry = carry
				remainder[r] = ele
			}
			if previousCarry != 0 { remainder.append(previousCarry) }

			remainder.setBit(at: 0, to: self.getBit(at: i))

			if !remainder.lessThan(divisor)
			{
				remainder.difference(divisor)
				quotient.setBit(at: i, to: true)
			}

			i -= 1
		}

		return (quotient, remainder)
	}

	/// Division with limbs, result is floored to nearest whole number.
	func dividing(_ divisor: Limbs) -> Limbs
	{
		return self.divMod(divisor).quotient
	}

	/// Modulo with limbs, result is floored to nearest whole number.
	func modulus(_ divisor: Limbs) -> Limbs
	{
		return self.divMod(divisor).remainder
	}

	//
	//
	//	MARK: - Limbs Comparing
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Limbs Comparing        |||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	//	Note:
	//	a < b iff b > a
	//	a <= b iff b >= a
	//	but:
	//	a < b iff !(a >= b)
	//	a <= b iff !(a > b)

	func lessThan(_ compare: Limbs) -> Bool
	{
		let lhsc = self.count
		let rhsc = compare.count

		if lhsc != rhsc
		{
			return lhsc < rhsc
		}

		var i = lhsc - 1
		while i >= 0
		{
			if self[i] != compare[i] { return self[i] < compare[i] }
			i -= 1
		}

		return false // lhs == rhs
	}

	func equalTo(_ compare: Limb) -> Bool
	{
		return self[0] == compare && self.count == 1
	}
}
//
//
//	MARK: - Useful BInt math functions
//	————————————————————————————————————————————————————————————————————————————————————————————
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||        Useful BInt math functions        ||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	————————————————————————————————————————————————————————————————————————————————————————————
//
//
//
internal class BIntMath
{
	/// Returns true iff (2 ** exp) - 1 is a mersenne prime.
	static func isMersenne(_ exp: Int) -> Bool
	{
		var mersenne = Limbs(repeating: Limb.max, count: exp >> 6)

		if (exp % 64) > 0
		{
			mersenne.append((Limb(1) << Limb(exp % 64)) - Limb(1))
		}

		var res: Limbs = [4]

		for _ in 0..<(exp - 2)
		{
			res = res.squared().differencing([2]).divMod(mersenne).remainder
		}

		return res.equalTo(0)
	}

	fileprivate static func euclid(_ a: Limbs, _ b: Limbs) -> Limbs
	{
		var (a, b) = (a, b)
		while !b.equalTo(0)
		{
			(a, b) = (b, a.divMod(b).remainder)
		}
		return a
	}

	fileprivate static func gcdFactors(_ lhs: Limbs, rhs: Limbs) -> (ax: Limbs, bx: Limbs)
	{
		let gcd = steinGcd(lhs, rhs)
		return (lhs.divMod(gcd).quotient, rhs.divMod(gcd).quotient)
	}

	public static func steinGcd(_ a: Limbs, _ b: Limbs) -> Limbs
	{
		if a == [0] { return b }
		if b == [0] { return a }

		// Trailing zeros
		var (za, zb) = (0, 0)

		while !a.getBit(at: za) { za += 1 }
		while !b.getBit(at: zb) { zb += 1 }

		let k = min(za, zb)

		var (a, b) = (a, b)
		a.shiftDown(za)
		b.shiftDown(k)

		repeat
		{
			zb = 0
			while !b.getBit(at: zb) { zb += 1 }
			b.shiftDown(zb)

			if b.lessThan(a) { (a, b) = (b, a) }
			// At this point, b >= a
			b.difference(a)
		}
		while b != [0]

		return a.shiftingUp(k)
	}

	public static func gcd(_ a: BInt, _ b: BInt) -> BInt
	{
		let limbRes = steinGcd(a.limbs, b.limbs)
		return BInt(sign: a.sign && !limbRes.equalTo(0), limbs: limbRes)
	}

	/// Do not use this, extremely slow. Only for testing purposes.
	public static func gcdEuclid(_ a: BInt, _ b: BInt) -> BInt
	{
		let limbRes = euclid(a.limbs, b.limbs)
		return BInt(sign: a.sign && !limbRes.equalTo(0), limbs: limbRes)
	}

	fileprivate static func lcmPositive(_ a: Limbs, _ b: Limbs) -> Limbs
	{
		return a.divMod(steinGcd(a, b)).quotient.multiplyingBy(b)
	}

	static func lcm(_ a:BInt, _ b:BInt) -> BInt
	{
		return BInt(limbs: lcmPositive(a.limbs, b.limbs))
	}

	static func fib(_ n:Int) -> BInt
	{
		var a: Limbs = [0], b: Limbs = [1], t: Limbs

		for _ in 2...n
		{
			t = b
			b.addLimbs(a)
			a = t
		}

		return BInt(limbs: b)
	}

	///	Order matters, repetition not allowed.
	static func permutations(_ n: Int, _ k: Int) -> BInt
	{
		// n! / (n-k)!
		return BInt(n).factorial() / BInt(n - k).factorial()
	}

	///	Order matters, repetition allowed.
	static func permutationsWithRepitition(_ n: Int, _ k: Int) -> BInt
	{
		// n ** k
		return BInt(n) ** k

	}

	///	Order does not matter, repetition not allowed.
	static func combinations(_ n: Int, _ k: Int) -> BInt
	{
		// (n + k - 1)! / (k! * (n - 1)!)
		return BInt(n + k - 1).factorial() / (BInt(k).factorial() * BInt(n - 1).factorial())
	}

	///	Order does not matter, repetition allowed.
	static func combinationsWithRepitition(_ n: Int, _ k: Int) -> BInt
	{
		// n! / (k! * (n - k)!)
		return BInt(n).factorial() / (BInt(k).factorial() * BInt(n - k).factorial())
	}

	static public func randomBInt(bits n: Int) -> BInt
	{
		let limbs = n >> 6
		let singleBits = n % 64

		var res = Limbs(repeating: 0, count: Int(limbs))

		for i in 0..<Int(limbs)
		{
			res[i] = Limb(UInt64.random(in: 0...UInt64.max))
		}
		if singleBits > 0
		{
			let last = Limb(UInt64.random(in: 0..<(1 << singleBits)))
			if last != 0 { res.append(last) }
		}

		return BInt(limbs: res)
	}
	let random = randomBInt

	func isPrime(_ n: BInt) -> Bool
	{
		if n <= 3 { return n > 1 }

		if ((n % 2) == 0) || ((n % 3) == 0) { return false }

		var i = 5
		while (i * i) <= n
		{
			if ((n % i) == 0) || ((n % (i + 2)) == 0)
			{
				return false
			}
			i += 6
		}
		return true
	}

	/// Quick exponentiation/modulo algorithm
	/// FIXME: for security, this should use the constant-time Montgomery algorithm to thwart timing attacks
	///
	/// - Parameters:
	///   - b: base
	///   - p: power
	///   - m: modulus
	/// - Returns: pow(b, p) % m
	static func mod_exp(_ b: BInt, _ p: BInt, _ m: BInt) -> BInt {
		precondition(m != 0, "modulus needs to be non-zero")
		precondition(p >= 0, "exponent needs to be non-negative")
		var base = b % m
		var exponent = p
		var result = BInt(1)
		while exponent > 0 {
			if exponent.limbs[0] % 2 != 0 {
				result = result * base % m
			}
			exponent.limbs.shiftDown(1)
			base *= base
			base %= m
		}
		return result
	}

	/// Non-negative modulo operation
	///
	/// - Parameters:
	///   - a: left hand side of the module operation
	///   - m: modulus
	/// - Returns: r := a % b such that 0 <= r < abs(m)
	static func nnmod(_ a: BInt, _ m: BInt) -> BInt {
		let r = a % m
		guard r.isNegative() else { return r }
		let p = m.isNegative() ? r - m : r + m
		return p
	}

	/// Convenience function combinding addition and non-negative modulo operations
	///
	/// - Parameters:
	///   - a: left hand side of the modulo addition
	///   - b: right hand side of the modulo addition
	///   - m: modulus
	/// - Returns: nnmod(a + b, m)
	static func mod_add(_ a: BInt, _ b: BInt, _ m: BInt) -> BInt {
		return nnmod(a + b, m)
	}
}
//
//
//	MARK: - BDouble
//	————————————————————————————————————————————————————————————————————————————————————————————
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||        BDouble        |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//	————————————————————————————————————————————————————————————————————————————————————————————
//
//
//
public struct BDouble:
	ExpressibleByIntegerLiteral,
	ExpressibleByFloatLiteral,
	CustomStringConvertible,
	SignedNumeric,
	Comparable,
	Hashable,
	Codable
{
	//
	//
	//	MARK: - Internal data
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Internal data        |||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public internal(set) var sign = Bool()
	public internal(set) var numerator = Limbs()
	public internal(set) var denominator = Limbs()

	public typealias Magnitude = Double
	public var magnitude: Double = 0.0

	//
	//
	//	MARK: - Initializers
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Initializers        ||||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public init?<T>(exactly source: T) where T : BinaryInteger
	{
		let i = BInt(source)
		self.init(i)
	}
	
	 public init(_ src: BInt) {
		self.init(src, over: BInt(1))
	}

	/**
		Inits a BDouble with two Limbs as numerator and denominator

		- Parameters:
		- numerator: The upper part of the fraction as Limbs
		- denominator: The lower part of the fraction as Limbs

		Returns: A new BDouble
	*/
	public init(sign: Bool, numerator: Limbs, denominator: Limbs)
	{
		precondition(
			!denominator.equalTo(0) && denominator != [] && numerator != [],
			"Denominator can't be zero and limbs can't be []"
		)

		self.sign = sign
		self.numerator = numerator
		self.denominator = denominator

		self.minimize()
	}

	public init(_ numerator: BInt, over denominator: BInt)
	{
		self.init(
			sign:			numerator.sign != denominator.sign,
			numerator:		numerator.limbs,
			denominator:	denominator.limbs
		)
	}

	public init(_ numerator: Int, over denominator: Int)
	{
		self.init(
			sign: (numerator < 0) != (denominator < 0),
			numerator: [UInt64(abs(numerator))],
			denominator: [UInt64(abs(denominator))]
		)
	}

	public init?(_ numerator: String, over denominator: String)
	{
		if let n = BInt(numerator) {
			if let d = BInt(denominator) {
				self.init(n, over: d)
				return
			}
		}
		return nil
	}

	public init?(_ nStr: String)
	{
		if let bi = BInt(nStr) {
			self.init(bi, over: 1)
		} else {
            if let exp = nStr.firstIndex(of: "e")?.utf16Offset(in: nStr)
			{
				let beforeExp = String(Array(nStr)[..<exp].filter{ $0 != "." })
				var afterExp = String(Array(nStr)[(exp + 1)...])
				var sign = false

				if let neg = afterExp.firstIndex(of: "-")?.utf16Offset(in: afterExp)
				{
					afterExp = String(Array(afterExp)[(neg + 1)...])
					sign = true
				}

				if sign
				{
					if var safeAfterExp = Int(afterExp) {
                        if beforeExp.starts(with: "+") || beforeExp.starts(with: "-") {
                            safeAfterExp = safeAfterExp - beforeExp.count + 2
                        } else {
                            safeAfterExp = safeAfterExp + (beforeExp.count - 1)
                        }
						// if safeAfterExp is negative this results in a crash
						// more testing and test cases needed
						if safeAfterExp < 0 {
							safeAfterExp = abs(safeAfterExp)
						}
						
						let den = ["1"] + [Character](repeating: "0", count: safeAfterExp)
						self.init(beforeExp, over: String(den))
						return
					}
					return nil
				}
				else
				{
					if var safeAfterExp = Int(afterExp) {
                        if beforeExp.starts(with: "+") || beforeExp.starts(with: "-") {
                            safeAfterExp = safeAfterExp - beforeExp.count + 2
                        } else {
                            safeAfterExp = safeAfterExp - (beforeExp.count - 1)
                        }
						var num: String
						if safeAfterExp >= 0 {
							num = beforeExp + String([Character](repeating: "0", count: safeAfterExp))
							self.init(num, over: "1")
						} else {
							num = beforeExp
							self.init(num, over: "1" + String(repeating: "0", count: abs(safeAfterExp)))
						}
						return
					}
					return nil
				}
			}

			if let io = nStr.firstIndex(of: ".")
			{
				let beforePoint = String(nStr[..<io])
				let afterPoint  = String(nStr[nStr.index(io, offsetBy: 1)...])

				if afterPoint == "0"
				{
					self.init(beforePoint, over: "1")
				}
				else
				{
					let den = ["1"] + [Character](repeating: "0", count: afterPoint.count)
					self.init(beforePoint + afterPoint, over: String(den))
				}
			}
			else
			{
				return nil
			}
		}
	}

	/// Create an instance initialized to a string with the value of mathematical numerical system of the specified radix (base).
	/// So for example, to get the value of hexadecimal string radix value must be set to 16.
	public init?(_ nStr: String, radix: Int)
	{
		if radix == 10 {
			// regular string init is faster
			// see metrics
			self.init(nStr)
			return
		}
		
		var useString = nStr
		if radix == 16 {
			if useString.hasPrefix("0x") {
				useString = String(nStr.dropFirst(2))
			}
		}
		
		if radix == 8 {
			if useString.hasPrefix("0o") {
				useString = String(nStr.dropFirst(2))
			}
		}
		
		if radix == 2 {
			if useString.hasPrefix("0b") {
				useString = String(nStr.dropFirst(2))
			}
		}
		
		let bint16 = BDouble(radix)
		
		var total = BDouble(0)
		var exp = BDouble(1)
		
		for c in useString.reversed() {
			let int = Int(String(c), radix: radix)
			if int != nil {
				let value =  BDouble(int!)
				total = total + (value * exp)
				exp = exp * bint16
			} else {
				return nil
			}
		}
		
		self.init(String(describing:total))
		
	}
	
	public init(_ z: Int)
	{
		self.init(z, over: 1)
	}

	/// Warning: Due to the inprecision of Double beware that this might effect the precision of your BDouble
	public init(_ d: Double)
	{
		let nStr = String(d)

		self.init(nStr)!
	}

	public init(integerLiteral value: Int)
	{
		self.init(value)
	}

	public init(floatLiteral value: Double)
	{
		self.init(value)
	}

	//
	//
	//	MARK: - Descriptions
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        Descriptions        ||||||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	/**
	 * returns the current value in a fraction format
	 */
	public var description: String
	{
		return self.fractionDescription
	}

	/**
	 * returns the current value in a fraction format
	 */
	public var fractionDescription : String
	{
		var res = (self.sign ? "-" : "")

		res.append(self.numerator.decimalRepresentation)

		if self.denominator != [1]
		{
			res.append("/".appending(self.denominator.decimalRepresentation))
		}

		return res
	}

	static private var _precision = 4
	/**
	 * the global percision for all newly created values
	 */
	static public var precision : Int
	{
		get
		{
			return _precision
		}
		set
		{
			var nv = newValue
			if nv < 0 {
				nv = 0
			}
			_precision = nv
		}
	}
	private var _precision : Int = BDouble.precision
	
	/**
	 * the precision for the current value
	 */
	public var precision : Int
	{
		get
		{
			return _precision
		}
		set
		{
		var nv = newValue
		if nv < 0 {
		nv = 0
		}
		_precision = nv
		}
	}

	/**
	 * returns the current value in decimal format with the current precision
	 */
	public var decimalDescription : String
	{
		return self.decimalExpansion(precisionAfterDecimalPoint: self.precision)
	}

	/**
	 Returns the current value in decimal format (always with a decimal point).
	 - parameter precision: the precision after the decimal point
	 - parameter rounded: whether or not the return value's last digit will be rounded up
	 */
    public func decimalExpansion(precisionAfterDecimalPoint precision: Int, rounded : Bool = true) -> String
    {
        var currentPrecision = precision

        if(rounded && precision > 0) {
            currentPrecision = currentPrecision + 1
        }

        let multiplier = [10].exponentiating(currentPrecision)
        let limbs = self.numerator.multiplyingBy(multiplier).divMod(self.denominator).quotient
        var res = BInt(limbs: limbs).description

        if currentPrecision <= res.count
        {
			res.insert(".", at: res.index(res.startIndex, offsetBy: res.count - currentPrecision))
            if res.hasPrefix(".") { res = "0" + res }
            else if res.hasSuffix(".") { res += "0" }
        }
        else
        {
            res = "0." + String(repeating: "0", count: currentPrecision - res.count) + res
        }

        var retVal = self.isNegative() && !limbs.equalTo(0) ? "-" + res : res

        if(rounded && precision > 0) {

            let lastDigit = Int(retVal.suffix(1))! // this should always be a number
            let secondDigit = retVal.suffix(2).prefix(1) // this could be a decimal

            retVal = String(retVal.prefix(retVal.count-2))
            if (secondDigit != ".") {
                if lastDigit >= 5 {
                    retVal = retVal + String(Int(secondDigit)! + 1)
                } else {
                    retVal = retVal + String(Int(secondDigit)!)
                }
            } else {
                retVal = retVal + "." + String(lastDigit)
            }
        }

        return retVal
    }

	public func hash(into hasher: inout Hasher) {
		hasher.combine("\(self.sign)\(self.numerator)\(self.denominator)")
	}

	/**
	 * Returns the size of the BDouble in bits.
     */
	public var size: Int
	{
		return 1 + ((self.numerator.count + self.denominator.count) * MemoryLayout<Limb>.size * 8)
	}

	/**
	 * Returns a formated human readable string that says how much space
	 * (in bytes, kilobytes, megabytes, or gigabytes) the BDouble occupies
	*/
	public var sizeDescription: String
	{
		// One bit for the sign, plus the size of the numerator and denominator.
		let bits = self.size

		if bits < 8_000
		{
			return String(format: "%.1f b", Double(bits) / 8.0)
		}
		if bits < 8_000_000
		{
			return String(format: "%.1f kb", Double(bits) / 8_000.0)
		}
		if UInt64(bits) < UInt64(8_000_000_000.0)
		{
			return String(format: "%.1f mb", Double(bits) / 8_000_000.0)
		}
		return String(format: "%.1f gb", Double(bits) / 8_000_000_000.0)
	}

	public func rawData() -> (sign: Bool, numerator: [UInt64], denominator: [UInt64])
	{
		return (self.sign, self.numerator, self.denominator)
	}

	/**
	 * - returns: `True` if positive, `False` otherwise
	 */
	public func isPositive() -> Bool { return !self.sign }
	/**
	 * - returns: `True` if negative, `False` otherwise
	*/
	public func isNegative() -> Bool { return self.sign }
	/**
	 * - returns: `True` if 0, `False` otherwise
	*/
	public func isZero() -> Bool { return self.numerator.equalTo(0) }

	public mutating func minimize()
	{
		if self.numerator.equalTo(0)
		{
			self.denominator = [1]
			return
		}

		let gcd = BIntMath.steinGcd(self.numerator, self.denominator)

		if gcd[0] > 1 || gcd.count > 1
		{
			self.numerator = self.numerator.divMod(gcd).quotient
			self.denominator = self.denominator.divMod(gcd).quotient
		}
	}

	/**
	 * If the right side of the decimal is greater than 0.5 then it will round up (ceil),
	 * otherwise round down (floor) to the nearest BInt
	 */
	public func rounded() -> BInt
	{
		if self.isZero() {
			return BInt(0)
		}
		let digits = 3
		let multiplier = [10].exponentiating(digits)

		let rawRes = abs(self).numerator.multiplyingBy(multiplier).divMod(self.denominator).quotient

		let res = BInt(limbs: rawRes).description

		let offset = res.count - digits
		let rhs = Double("0." + res.suffix(res.count - offset))!
		var lhs: String.SubSequence
		if offset >= 0 {
			lhs = res.prefix(offset)
		} else {
			lhs = ""
		}
		var retVal = BInt(String(lhs))!
		
		if self.isNegative()
		{
			retVal = -retVal
			if rhs > 0.5 {
				retVal = retVal - BInt(1)
			}
		} else {
			if rhs > 0.5
			{
				retVal = retVal + 1
			}
		}

		return retVal
	}

	/**
	 * The power of 1/root
	 *
	 * - warning: This may take a while. This is only precise up until precision. When comparing results after this function ` use` nearlyEqual`
	 */
	public func nthroot(_ root: Int) -> BDouble
	{
		return self ** BDouble(BInt(1), over: BInt(root))
	}
	
	/**
	 * The square root
	 *
	 * - warning: This may take a while. This is only precise up until precision. When comparing results after this function ` use` nearlyEqual`
	 */
	public func squareRoot() -> BDouble
	{
		return self ** BDouble(BInt(1), over: BInt(2))
	}

	//
	//
	//	MARK: - BDouble Addition
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BDouble Addition        ||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public static func +(lhs: BDouble, rhs: BDouble) -> BDouble
	{
		// a/b + c/d = ad + bc / bd, where lhs = a/b and rhs = c/d.
		let ad = lhs.numerator.multiplyingBy(rhs.denominator)
		let bc = rhs.numerator.multiplyingBy(lhs.denominator)
		let bd = lhs.denominator.multiplyingBy(rhs.denominator)

		let resNumerator = BInt(sign: lhs.sign, limbs: ad) + BInt(sign: rhs.sign, limbs: bc)

		return BDouble(
			sign: resNumerator.sign && !resNumerator.limbs.equalTo(0),
			numerator: resNumerator.limbs,
			denominator: bd
		)
	}

	public static func +(lhs: BDouble, rhs: Double) -> BDouble { return lhs + BDouble(rhs) }
	public static func +(lhs: Double, rhs: BDouble) -> BDouble { return BDouble(lhs) + rhs }
	public static func +(lhs: BDouble, rhs: BInt) -> BDouble { return lhs + BDouble(rhs) }
	public static func +(lhs: BInt, rhs: BDouble) -> BDouble { return BDouble(lhs) + rhs }
	
	public static func +=(lhs: inout BDouble, rhs: BDouble) {
		let res = lhs + rhs
		lhs = res
	}

	public static func +=(lhs: inout BDouble, rhs: Double) { lhs += BDouble(rhs) }


	//
	//
	//	MARK: - BDouble Negation
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BDouble Negation        ||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	/**
	 * makes the current value negative
	 */
	public mutating func negate()
	{
		if !self.isZero()
		{
			self.sign = !self.sign
		}
	}

	public static prefix func -(n: BDouble) -> BDouble
	{
		var n = n
		n.negate()
		return n
	}

	//
	//
	//	MARK: - BDouble Subtraction
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BDouble Subtraction        |||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public static func -(lhs: BDouble, rhs: BDouble) -> BDouble
	{
		return lhs + -rhs
	}
	public static func -(lhs: BDouble, rhs: Double) -> BDouble { return lhs - BDouble(rhs) }
	public static func -(lhs: Double, rhs: BDouble) -> BDouble { return BDouble(lhs) - rhs }
	public static func -(lhs: BDouble, rhs: BInt) -> BDouble { return lhs - BDouble(rhs) }
	public static func -=(lhs: inout BDouble, rhs: BDouble) {
		let res = lhs - rhs
		lhs = res
	}

	public static func -=(lhs: inout BDouble, rhs: Double) { lhs -= BDouble(rhs) }

	//
	//
	//	MARK: - BDouble Multiplication
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BDouble Multiplication        ||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public static func *(lhs: BDouble, rhs: BDouble) -> BDouble
	{
		var res =  BDouble(
			sign:			lhs.sign != rhs.sign,
			numerator:		lhs.numerator.multiplyingBy(rhs.numerator),
			denominator:	lhs.denominator.multiplyingBy(rhs.denominator)
		)

		if res.isZero() { res.sign = false }
		return res
	}
	public static func *(lhs: BDouble, rhs: Double) -> BDouble { return lhs * BDouble(rhs) }
	public static func *(lhs: Double, rhs: BDouble) -> BDouble { return BDouble(lhs) * rhs }
	public static func *(lhs: BDouble, rhs: BInt) -> BDouble { return lhs * BDouble(rhs) }
	public static func *(lhs: BInt, rhs: BDouble) -> BDouble { return BDouble(lhs) * rhs }

	public static func *=(lhs: inout BDouble, rhs: BDouble) {
		let res = lhs * rhs
		lhs = res
	}

	public static func *=(lhs: inout BDouble, rhs: Double) { lhs *= BDouble(rhs) }

	//
	//
	//	MARK: - BDouble Exponentiation
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BDouble Exponentiation        ||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public static func **(_ base : BDouble, _ exponent : Int) -> BDouble
	{
		if exponent == 0
		{
			return BDouble(1)
		}
		if exponent == 1
		{
			return base
		}
		if exponent < 0
		{
			return BDouble(1) / (base ** -exponent)
		}

		return base * (base ** (exponent - 1))
	}
	
	public static func **(_ base: BDouble, _ exponent: BInt) -> BDouble
	{
		if exponent == 0
		{
			return BDouble(1)
		}
		if exponent == 1
		{
			return base
		}
		if exponent < 0
		{
			return BDouble(1) / (base ** -exponent)
		}

		return base * (base ** (exponent - 1))
	}
	
	/**
	 * - reference: http://rosettacode.org/wiki/Nth_root
	 */
	public static func **(_ base: BDouble, _ exponent: BDouble) -> BDouble
	{
		var count = base.precision
		
		// something over 1
		if BInt(limbs: exponent.denominator) == 1 {
			return base**BInt(sign: exponent.sign, limbs: exponent.numerator)
		}
		
		if BInt(limbs: exponent.numerator) != 1 {
			return (base ** BInt(sign: exponent.sign, limbs: exponent.numerator)) ** BDouble(sign: false, numerator: BDouble(1).numerator, denominator: exponent.denominator)
		}
		
		// we have 1/something
		
		var previous  = BDouble(1)
		var ans = previous
		let exp = BInt(sign: exponent.sign, limbs: exponent.denominator)
		let prec = BDouble(0.1) ** (abs(base.precision) + 1)
		
		while(true) {
			previous = ans
			
			let rlhs = BDouble(BInt(1), over:exp)
			let rrhs = ((exp-1)*ans + (base / pow(ans, exp-1)))
			ans = rlhs * rrhs

			if abs(ans-previous) < prec {
				break
			}

			count = count + 1
		}
		
		return ans
	}

	//
	//
	//	MARK: - BDouble Division
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BDouble Division        ||||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	public static func /(lhs: BDouble, rhs: BDouble) -> BDouble
	{
		var res =  BDouble(
			sign:			lhs.sign != rhs.sign,
			numerator:		lhs.numerator.multiplyingBy(rhs.denominator),
			denominator:	lhs.denominator.multiplyingBy(rhs.numerator)
		)

		if res.isZero() { res.sign = false }
		return res
	}
	public static func /(lhs: BDouble, rhs: Double) -> BDouble { return lhs / BDouble(rhs) }
	public static func /(lhs: BDouble, rhs: BInt) -> BDouble { return lhs / BDouble(rhs) }
	public static func /(lhs: Double, rhs: BDouble) -> BDouble { return BDouble(lhs) / rhs }
	
	public static func %(lhs: BDouble, rhs:BDouble) -> BDouble { return mod(lhs, rhs) }
	public static func %(lhs: BDouble, rhs:Double) -> BDouble { return mod(lhs, BDouble(rhs)) }
	public static func %(lhs: Double, rhs:BDouble) -> BDouble { return mod(BDouble(lhs), rhs) }

	//
	//
	//	MARK: - BDouble Comparing
	//	————————————————————————————————————————————————————————————————————————————————————————
	//	||||||||        BDouble Comparing        |||||||||||||||||||||||||||||||||||||||||||||||
	//	————————————————————————————————————————————————————————————————————————————————————————
	//
	//
	//

	/**
	* An == comparison with an epsilon (fixed then a calculated "ULPs")
	 * Reference: http://floating-point-gui.de/errors/comparison/
	 * Reference: https://bitbashing.io/comparing-floats.html
	 */
	public static func nearlyEqual(_ lhs: BDouble, _ rhs: BDouble, epsilon: Double = 0.00001) -> Bool {
		let absLhs = abs(lhs)
		let absRhs = abs(rhs);
		let diff = abs(lhs - rhs);
		
		if (lhs == rhs) { // shortcut, handles infinities
			return true;
		} else if diff <= epsilon {
			return true // shortcut
    } else if (lhs.isZero() || rhs.isZero() || diff < Double.leastNormalMagnitude) {
			// lhs or rhs is zero or both are extremely close to it
			// relative error is less meaningful here
			return diff < (epsilon * Double.leastNormalMagnitude);
		} else { // use relative error
			return diff / min((absLhs + absRhs), BDouble(Double.greatestFiniteMagnitude)) < epsilon;
		}
	}
	
	public static func ==(lhs: BDouble, rhs: BDouble) -> Bool
	{
		if lhs.sign != rhs.sign { return false }
		if lhs.numerator != rhs.numerator { return false }
		if lhs.denominator != rhs.denominator { return false }

		return true
	}
	public static func ==(lhs: BDouble, rhs: Double) -> Bool { return lhs == BDouble(rhs) }
	public static func ==(lhs: Double, rhs: BDouble) -> Bool { return BDouble(lhs) == rhs }

	public static func !=(lhs: BDouble, rhs: BDouble) -> Bool
	{
		return !(lhs == rhs)
	}
	public static func !=(lhs: BDouble, rhs: Double) -> Bool { return lhs != BDouble(rhs) }
	public static func !=(lhs: Double, rhs: BDouble) -> Bool { return BDouble(lhs) != rhs }

	public static func <(lhs: BDouble, rhs: BDouble) -> Bool
	{
		if lhs.sign != rhs.sign { return lhs.sign }

		// more efficient than lcm version
		let ad  = lhs.numerator.multiplyingBy(rhs.denominator)
		let bc = rhs.numerator.multiplyingBy(lhs.denominator)

		if lhs.sign { return bc.lessThan(ad) }

		return ad.lessThan(bc)
	}
	public static func <(lhs: BDouble, rhs: Double) -> Bool { return lhs < BDouble(rhs) }
	public static func <(lhs: Double, rhs: BDouble) -> Bool { return BDouble(lhs) < rhs }

	public static func >(lhs: BDouble, rhs: BDouble) -> Bool { return rhs < lhs }
	public static func >(lhs: BDouble, rhs: Double) -> Bool { return lhs > BDouble(rhs) }
	public static func >(lhs: Double, rhs: BDouble) -> Bool { return BDouble(lhs) > rhs }

	public static func <=(lhs: BDouble, rhs: BDouble) -> Bool { return !(rhs < lhs) }
	public static func <=(lhs: BDouble, rhs: Double) -> Bool { return lhs <= BDouble(rhs) }
	public static func <=(lhs: Double, rhs: BDouble) -> Bool { return BDouble(lhs) <= rhs }

	public static func >=(lhs: BDouble, rhs: BDouble) -> Bool { return !(lhs < rhs) }
	public static func >=(lhs: BDouble, rhs: Double) -> Bool { return lhs >= BDouble(rhs) }
	public static func >=(lhs: Double, rhs: BDouble) -> Bool { return BDouble(lhs) >= rhs }
}

//
//
//	MARK: - BDouble Operators
//	————————————————————————————————————————————————————————————————————————————————————————————
//	||||||||        BDouble more Operators        ||||||||||||||||||||||||||||||||||||||||||||||
//	————————————————————————————————————————————————————————————————————————————————————————————
//
//
//

/**
 * Returns the absolute value of the given number.
 * - parameter x: a big double
 */
public func abs(_ x: BDouble) -> BDouble
{
	return BDouble(
		sign: false,
		numerator: x.numerator,
		denominator: x.denominator
	)
}

/**
 * round to largest BInt value not greater than base
 */
public func floor(_ base: BDouble) -> BInt
{
	if base.isZero()
	{
		return BInt(0)
	}
	
	let digits = 3
	let multiplier = [10].exponentiating(digits)

	let rawRes = abs(base).numerator.multiplyingBy(multiplier).divMod(base.denominator).quotient

	let res = BInt(limbs: rawRes).description
	
	let offset = res.count - digits
	
	var lhs: String
	if offset >= 0 {
		lhs = res.prefix(offset).description
	} else {
		lhs = ""
	}
	let rhs = Double("0." + res.suffix(res.count - offset))!
	
	var ans = BInt(String(lhs))!
	if base.isNegative() {
		ans = -ans
		if rhs > 0.0 {
			ans = ans - BInt(1)
		}
	}

	return ans
}

/**
 * round to smallest BInt value not less than base
 */
public func ceil(_ base: BDouble) -> BInt
{
	
	if base.isZero()
	{
		return BInt(0)
	}
	let digits = 3
	let multiplier = [10].exponentiating(digits)

	let rawRes = abs(base).numerator.multiplyingBy(multiplier).divMod(base.denominator).quotient

	let res = BInt(limbs: rawRes).description

	let offset = res.count - digits
	let rhs = Double("0." + res.suffix(res.count - offset))!
	var lhs: String.SubSequence
	if offset >= 0 {
		lhs = res.prefix(offset)
	} else {
		lhs = ""
	}
	
	var retVal = BInt(String(lhs))!
	
	if base.isNegative()
	{
		retVal = -retVal
	} else {
		if rhs > 0.0
		{
			retVal += 1
		}
	}
	
	return retVal
}

/**
 * Returns a BDouble number raised to a given power.
 * - warning: This may take a while
 */
public func pow(_ base : BDouble, _ exp : Int) -> BDouble {
	return base**exp
}

/**
 * Returns a BDouble number raised to a given power.
 * - warning: This may take a while
 */
public func pow(_ base : BDouble, _ exp : BInt) -> BDouble {
	return base**exp
}

/**
 * - warning: This may take a while. This is only precise up until precision. When comparing results after `pow` or `** ` use` nearlyEqual`
 */
public func pow(_ base : BDouble, _ exp : BDouble) -> BDouble {
	return base**exp
}

/**
 * Returns the BDouble that is the smallest
 */
public func min(_ lhs: BDouble, _ rhs: BDouble) -> BDouble {
	if lhs <= rhs {
		return lhs
	}
	return rhs
}

/**
 * Returns the BDouble that is largest
 */
public func max(_ lhs: BDouble, _ rhs: BDouble) -> BDouble {
	if lhs >= rhs {
		return lhs
	}
	return rhs
}

/**
 * Returns the modulo (remainder)
 */
public func mod(_ lhs: BDouble, _ rhs: BDouble) -> BDouble {
	precondition(!rhs.isZero(), "Right hand side cannot be zero")
	let inner_ceil: BDouble = -lhs/rhs
	let _ceil: BDouble = BDouble(ceil(inner_ceil))
	return lhs + (rhs*_ceil)
}
