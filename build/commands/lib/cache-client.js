const { pipeline } = require("stream");
const { promisify } = require("util");
const pipe = promisify(pipeline);

const cacheClient = () => {
  const {AWS_REGION: region, BRAVE_TEST_CACHE_S3_BUCKET: bucket} = process.env;
  if (!region || !bucket) 
    return null;

  const { 
    S3Client, 
    HeadObjectCommand, 
    GetObjectCommand
  } = require( "@aws-sdk/client-s3");
  const s3 = new S3Client({region: AWS_REGION });

  return {
    check(key) {
      return s3.send(new HeadObjectCommand({ 
        Bucket: bucket, 
        Key: key 
        })).then(()=>true).catch(()=>false)
    },
    
    async download(key, dest) {

      const response = await s3.send(new GetObjectCommand({
        Bucket: bucket,
        Key: key
      }));

      try {
        await pipe(response.Body, createWriteStream(dest));

        console.log(`Downloaded s3://${bucket}/${key} to ${destinationPath}`);
        return true;
      } catch (e) {
        console.error(e);
        return false;
      }
    },

    async upload(key, file) {
      const fileStream = fs.createReadStream(file);

      const uploadParams = {
        Bucket: bucket,
        Key: key,
        Body: fileStream,
        ContentType: "text/plain", // adjust based on your file type
      };

      await s3.send(new PutObjectCommand(uploadParams));

      console.log(`Uploaded ${file} to s3://${bucket}/${key}`);
    }
  }
}

module.exports = cacheClient;