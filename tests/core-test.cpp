#include <gtest/gtest.h>
#include "../core.h"

TEST(KeyGenTest, ProducesKeys) {
    auto kp = KeyGen();
    ASSERT_NE(kp.pk, nullptr);
    ASSERT_NE(kp.sk, nullptr);
    EXPECT_TRUE(kp.pk->isValid());
    EXPECT_TRUE(kp.sk->isValid());
}

TEST(KeyGenTest, PkAndSkShareModulus) {
    auto kp = KeyGen();
    EXPECT_EQ(kp.pk->getModulusHex(), kp.sk->getModulusHex());
}

TEST(FlowTest, SignAndVerify) {
    auto kp = KeyGen();
    std::string pk_str = kp.pk->toString();
    std::string sk_str = kp.sk->toString();
    std::string msg = "test message 123";

    std::string blind_data = Blind(pk_str, msg);
    auto parts = split_str(blind_data, ':');
    ASSERT_EQ(parts.size(), 2u);

    std::string blind_sig = BlindSign(sk_str, parts[0]);
    std::string sig = Finalize(pk_str, msg, blind_sig, parts[1]);

    EXPECT_TRUE(Verify(pk_str, msg, sig));
}

TEST(FlowTest, WrongMessageFails) {
    auto kp = KeyGen();
    std::string pk_str = kp.pk->toString();
    std::string sk_str = kp.sk->toString();

    std::string blind_data = Blind(pk_str, "real message");
    auto parts = split_str(blind_data, ':');
    std::string blind_sig = BlindSign(sk_str, parts[0]);
    std::string sig = Finalize(pk_str, "real message", blind_sig, parts[1]);

    EXPECT_FALSE(Verify(pk_str, "fake message", sig));
}

TEST(FlowTest, BadSigFails) {
    auto kp = KeyGen();
    std::string pk_str = kp.pk->toString();
    EXPECT_FALSE(Verify(pk_str, "hello", "deadbeef"));
}

TEST(OptionalTest, TryVerifyBadKey) {
    auto res = TryVerify("notakey", "msg", "sig");
    EXPECT_FALSE(res.has_value());
}

TEST(OptionalTest, TryVerifyGoodKey) {
    auto kp = KeyGen();
    std::string pk_str = kp.pk->toString();
    std::string sk_str = kp.sk->toString();

    std::string bd = Blind(pk_str, "hi");
    auto p = split_str(bd, ':');
    std::string bs = BlindSign(sk_str, p[0]);
    std::string sig = Finalize(pk_str, "hi", bs, p[1]);

    auto res = TryVerify(pk_str, "hi", sig);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res.value());
}

TEST(ExceptionTest, ParseExceptionOnBadKey) {
    EXPECT_THROW(Blind("bad", "msg"), ParseException);
}

TEST(ExceptionTest, ParseExceptionOnBadSk) {
    EXPECT_THROW(BlindSign("bad", "aabb"), ParseException);
}

TEST(PublicKeyTest, FromStringNulloptOnBad) {
    auto res = PublicKey::fromString("notvalidatall");
    EXPECT_FALSE(res.has_value());
}

TEST(PublicKeyTest, FromStringOkOnGood) {
    auto kp = KeyGen();
    auto res = PublicKey::fromString(kp.pk->toString());
    EXPECT_TRUE(res.has_value());
}

TEST(BigIntTest, AddBasic) {
    BigInt a(100), b(200);
    BigInt c = add(a, b);
    EXPECT_EQ(to_hex(c), to_hex(BigInt(300)));
}

TEST(BigIntTest, MulBasic) {
    BigInt a(12), b(13);
    BigInt c = mul(a, b);
    EXPECT_EQ(to_hex(c), to_hex(BigInt(156)));
}

TEST(BigIntTest, ModpowBasic) {
    BigInt base(2), exp(10), m(1000);
    BigInt res = modpow(base, exp, m);
    EXPECT_EQ(to_hex(res), to_hex(BigInt(1024 % 1000)));
}

TEST(BigIntTest, ModinvBasic) {
    BigInt a(3), m(7);
    BigInt inv = modinv(a, m);
    BigInt prod = bigmod(mul(a, inv), m);
    EXPECT_TRUE(prod.is_one());
}