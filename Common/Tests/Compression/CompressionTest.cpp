#include <gtest/gtest.h>

#include <cctype>
#include <cstring>
#include <string>
#include <vector>

#include "Compression.h"

namespace
{

	// Build a chunk of artificial test data. It mixes a highly repetitive section
	// (so the compressors have redundancy to exploit) with a deterministic
	// pseudo-random tail (so there is some entropy too). Deterministic so the test
	// is reproducible.
	std::vector<UnsignedByte> makeTestData()
	{
		std::vector<UnsignedByte> data;
		data.reserve(16 * 1024);

		const char *phrase = "The quick brown fox jumps over the lazy dog. ";
		for (int i = 0; i < 128; ++i)
			for (const char *p = phrase; *p; ++p)
				data.push_back((UnsignedByte)*p);

		UnsignedInt seed = 0x12345678u;
		for (int i = 0; i < 4096; ++i)
		{
			seed = seed * 1103515245u + 12345u;
			data.push_back((UnsignedByte)(seed >> 16));
		}

		return data;
	}

	std::vector<CompressionType> getCompressionTypes()
	{
		std::vector<CompressionType> types;
		for (int t = COMPRESSION_MIN; t <= COMPRESSION_MAX; ++t)
		{
			if (t == COMPRESSION_NONE)
				continue;
			types.push_back((CompressionType)t);
		}
		return types;
	}

	std::string getCompressionName(const ::testing::TestParamInfo<CompressionType> &info)
	{
		const std::string name = CompressionManager::getCompressionNameByType(info.param);
		std::string clean;
		for (char c : name)
		{
			if (std::isalnum((unsigned char)c))
				clean += c;
			else if (!clean.empty() && clean.back() != '_')
				clean += '_';
		}
		while (!clean.empty() && clean.back() == '_')
			clean.pop_back();
		return clean.empty() ? std::string("Type") + std::to_string((int)info.param) : clean;
	}

} // namespace

class CompressionRoundTrip : public ::testing::TestWithParam<CompressionType>
{
};

// Compressing then decompressing must reproduce the original data exactly.
TEST_P(CompressionRoundTrip, RestoresOriginalData)
{
	const CompressionType type = GetParam();
	const char *typeName = CompressionManager::getCompressionNameByType(type);

	const std::vector<UnsignedByte> original = makeTestData();
	const Int origLen = (Int)original.size();

	const Int maxCompressedSize = CompressionManager::getMaxCompressedSize(origLen, type);
	ASSERT_GT(maxCompressedSize, 0) << "No max compressed size for " << typeName;

	// compressData takes non-const void*, so work on a copy of the source.
	std::vector<UnsignedByte> src(original);
	std::vector<UnsignedByte> compressed(maxCompressedSize, 0);

	const Int compressedLen = CompressionManager::compressData(
		type, src.data(), origLen, compressed.data(), maxCompressedSize);
	ASSERT_GT(compressedLen, 0) << "Failed to compress with " << typeName;
	ASSERT_LE(compressedLen, maxCompressedSize) << "Overflowed buffer with " << typeName;

	// The compressed blob should self-identify as the type we used.
	EXPECT_TRUE(CompressionManager::isDataCompressed(compressed.data(), compressedLen)) << typeName;
	EXPECT_EQ(CompressionManager::getCompressionType(compressed.data(), compressedLen), type) << typeName;
	EXPECT_EQ(CompressionManager::getUncompressedSize(compressed.data(), compressedLen), origLen) << typeName;

	std::vector<UnsignedByte> decompressed(origLen, 0);
	const Int decompressedLen = CompressionManager::decompressData(
		compressed.data(), compressedLen, decompressed.data(), origLen);

	ASSERT_EQ(decompressedLen, origLen) << "Failed to decompress with " << typeName;
	EXPECT_EQ(0, memcmp(original.data(), decompressed.data(), origLen))
		<< "Round-tripped data differs from original with " << typeName;
}

INSTANTIATE_TEST_SUITE_P(
	Compression,
	CompressionRoundTrip,
	::testing::ValuesIn(getCompressionTypes()),
	getCompressionName);

TEST(Compression, UncompressedDataIsNotDetectedAsCompressed)
{
	const std::vector<UnsignedByte> data = makeTestData();
	EXPECT_FALSE(CompressionManager::isDataCompressed(data.data(), (Int)data.size()));
	EXPECT_EQ(CompressionManager::getCompressionType(data.data(), (Int)data.size()), COMPRESSION_NONE);
}
