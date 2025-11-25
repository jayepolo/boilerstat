# BoilerStat PostgreSQL Database

## Quick Start

1. **Configure credentials** (choose one method):

   **Option A: Use default credentials (for development)**
   ```bash
   docker-compose up -d
   ```

   **Option B: Use custom credentials**
   ```bash
   # Copy example environment file
   cp .env.example .env
   
   # Edit .env file with your secure passwords
   nano .env
   
   # Update docker-compose.yml to use .env file
   docker-compose up -d
   ```

2. **Start the database**:
   ```bash
   cd postgres-db
   docker-compose up -d
   ```

3. **Check status**:
   ```bash
   docker-compose logs postgres
   docker-compose ps
   ```

## Default Credentials

- **Database**: `boilerstat`
- **Username**: `boilerstat` 
- **Password**: `boilerstat123`
- **Port**: `5432`
- **Host**: `localhost`

## Connection Information

- **Connection String**: `postgresql://boilerstat:boilerstat123@localhost:5432/boilerstat`
- **Admin Access**: Use `psql` client or any PostgreSQL GUI tool

## Security Notes

⚠️ **Change the default password** in production:
1. Edit `docker-compose.yml` 
2. Change `POSTGRES_PASSWORD` value
3. Recreate container: `docker-compose down && docker-compose up -d`

## Management Commands

```bash
# Stop database
docker-compose down

# Stop and remove all data
docker-compose down -v

# View logs
docker-compose logs -f postgres

# Connect to database
docker-compose exec postgres psql -U boilerstat -d boilerstat
```